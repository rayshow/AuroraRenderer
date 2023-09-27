from array import array
from ast import Global, Param
from collections import deque
from copyreg import constructor
from distutils.log import info
from gettext import find
from glob import glob
from importlib.metadata import requires
from importlib.resources import contents
from multiprocessing.sharedctypes import Value
import os
import io
from platform import platform
from re import S
import sys
from unicodedata import category
from winreg import EnumKey
import xml.etree.ElementTree as XmlTree
from enum import Enum, EnumMeta, unique
from typing import Callable
from queue import Queue
from xmlrpc.client import Boolean

from setuptools import Command

python_requires=">=3.10"

def GetNodeAttrib(Name, Node):
    if Node!=None and Name in Node.attrib and Node.attrib[Name] !=None:
        return Node.attrib[Name].strip()
    return ''

def GetNodeText(Node):
    if Node!=None and Node.text!=None:
        return Node.text.strip()
    return ''

def GetNodeSubText(Node, Name):
    subNode =  Node.find(Name)
    if subNode!=None:
        return subNode.text
    return None

def GetTailText(Node):
    if Node!=None and Node.tail!=None:
        return Node.tail.strip()
    return ''

def WrapProtect(protect, content):
    if protect!='' and protect!=None:
        return f'\n#ifdef {protect}\n{content}\n#endif // {protect}'
    return content

def WrapIfElseProtect(protect, macroName, macroContent):
    defineMacro=macroContent
    emptyDefineMacro = f'\n#define {macroName}'
    contentDefineMacro = f'\n#define {macroName}{macroContent}'
    if protect!=None and protect!='':
        contentDefineMacro= f'#ifdef {protect}{contentDefineMacro}\n#else{emptyDefineMacro}\n#endif // {protect}'
    return contentDefineMacro
        

@unique
class ECommandType(Enum):
    Other=0
    Device=1
    Instance=2
    Global=3

@unique
class EArrayType(Enum):
    Other =0
    Array =1
    String =2
    StringArray=3
    _2DArray=4

@unique
class ETypeCategory(Enum):
    Invalid =0
    Enum=1
    Handle=2
    Struct=3
    FunPtr=4
    Include=5
    Require=6
    Define=7
    BaseType=8
    Typedef =9
    Union=10

@unique
class EEnumCategory(Enum):
    # normal enum , value is 0-N, can write as Array[EnumMax]
    Enum=0
    # normal enum bit mask
    BitMask =1
    # big enum
    BitMask64 = 2
    # value contain part ordered value like 0-N and disorder extend -1,-2 etc.
    EnumExtend=3
    # enum just for some value collections
    ValueCollections=4
    # const
    ConstValue=5

def GetArrayType(lengthParameter:str):
    #if lengthParameter=='geometryCount,1':
    #    print('')

    if lengthParameter == None or lengthParameter=='':
        return EArrayType.Other,''
    lengthArray = lengthParameter.split(',')
    if lengthParameter =='null-terminated':
        return EArrayType.String,''
    elif len(lengthArray) == 1:
        return EArrayType.Array,lengthArray
    elif len(lengthArray) == 2:
        if lengthArray[1] == 'null-terminated':
            return EArrayType.StringArray,lengthArray
        else:
            return EArrayType._2DArray,lengthArray
    raise ValueError(f'unkown array type of parameter {lengthParameter}')

class VkMember:
    optional:bool 
    type:str=None 
    name:str=None
    pointer:str=None
    array:str =None 
    prefix:str=None 

    # array type
    lengthParameters:list[str]=None
    arrayCategory:EArrayType=EArrayType.Other

    # refer to the length member 
    lengthMember = None

    # is length of other member
    bIsLength = False
    altlen =''
    
    def __init__(self, rootNode) -> None:
        typeNode = rootNode.find('type')
        nameNode = rootNode.find('name')
        self.type = GetNodeText(typeNode) 
        self.name = GetNodeText(nameNode) 

        lengthParameter = GetNodeAttrib('len', rootNode)
        self.altlen = GetNodeAttrib('altlen', rootNode)
        if self.altlen!='':
            lengthParameter=self.altlen
        self.arrayCategory, self.lengthParameters = GetArrayType(lengthParameter)

        optional = GetNodeAttrib('optional', rootNode)
        if optional =='true':
            self.optional = True 
        else:
            self.optional = False

        self.prefix = GetNodeText(rootNode)
        self.pointer = GetTailText(typeNode)
        self.array = GetTailText(nameNode)


    def getDeclare(self)->str:        
        return f"{self.type} {self.prefix}{self.pointer} {self.name}{self.array}"

    def getFullType(self)->str:
        return f"{self.type} {self.prefix}{self.pointer}"
    

    def __str__(self) -> str:
        return self.getDeclare()+" optional:{self.optional}"

# a member in vk struct
class VkTypeMember(VkMember):
    values:str=None
    limittype:str=None
    parent:str=None
 
    def __init__(self, rootNode) -> None:
        super().__init__(rootNode)
        self.parent = GetNodeAttrib('values', rootNode)
        self.limittype = GetNodeAttrib('limittype', rootNode)
        self.values = GetNodeAttrib('values', rootNode)

# a member in vk command
class VkCommandMember(VkMember):
    successCodes:list[str]=None
    errorCodes:list[str]=None 
    def __init__(self, rootNode) -> None:
        super().__init__(rootNode)
        self.successCodes = GetNodeAttrib('successcodes', rootNode)
        self.errorCodes = GetNodeAttrib('errorCodes', rootNode)

class VkCommand:
    name:str=None
    commandType:ECommandType=None
    ret:str =None
    params:dict[str, VkCommandMember]=None
    bIsPod:bool=True

    protect:str=''

    def __str__(self) -> str:
        paramStr =''
        index =0
        for param in self.params:
            if index != 0:
                paramStr+=","
            paramStr += str(param)
            index=index+1

        return f"{self.ret} {self.name}({paramStr})"

    def __init__(self, name:str, rootNode):
        global GlobalCommands
        commandType=ECommandType.Other
        self.name = name
        self.ret = GetNodeSubText(rootNode, 'proto/type') 
        self.params = { member.name:member for member in ( VkCommandMember(paramNode) for paramNode in rootNode.findall('param') ) }
        if len(self.params)==0:
             raise ValueError('VkCommand{name} construct param is empty')

        index= 0
        for paramName, param in self.params.items():
            if index ==0:
                firstType = param.type
            
            # get addr from dll/so
            GlobalCommands = [
            'vkCreateInstance', 
            'vkGetInstanceProcAddr',
            #'vkGetDeviceProcAddr',
            'vkEnumerateInstanceExtensionProperties',
            'vkEnumerateInstanceLayerProperties' ]
            
            if name in GlobalCommands:
                commandType= ECommandType.Global
            elif firstType in ('VkInstance', 'VkPhysicalDevice'):
                commandType = ECommandType.Instance
            elif firstType in ( 'VkDevice' ,'VkQueue' , 'VkCommandBuffer'):
                commandType = ECommandType.Device
            else:
                if name.find('Instance')!=-1:
                    commandType = ECommandType.Instance
                elif name.find('Device')!=-1:
                    commandType = ECommandType.Device
                else:
                    commandType = ECommandType.Other
        
            self.commandType = commandType
            index=index+1

            if param.lengthParameters!=None and len(param.lengthParameters)>0:
                if param.lengthParameters[0] in self.params:
                    param.lengthMember = self.params[param.lengthParameters[0]]
                    param.lengthMember.bIsLength = True
                    if param.arrayCategory==EArrayType.Other:
                        raise ValueError(f"param {param.name} category error")
                if len(param.lengthParameters)>1:
                    if len(param.lengthParameters) !='null-terminated':
                        raise ValueError(f"param {param.name} category error")

    def getDeclare(self):
        paramDecl=''
        typeDecl=''
        valueDecl=''
        index= 0
        for param in self.params.values():
            type=param.getFullType()
            value=param.name
            array=param.array
            comma=''
            if index != 0:
                comma=', '
            index=index+1
            paramDecl+=f'{comma}{type} {value}{array}'
            typeDecl+=f'{comma}{type}{array}'
            valueDecl+=f'{comma}{value}'
        return (paramDecl, typeDecl, valueDecl)

class VkType:
    # optional
    name:str=None

    # optional
    parent:str=None 

    # requires
    requires:str=None 

    # enum / handle / struct / function 
    category:ETypeCategory=ETypeCategory.Invalid

    alias:str=None 

    # tree
    depends:list[str]=None 


    # mmeber
    members:dict[str,VkTypeMember]=None

    # sType values
    sType:str=None

    # object type
    objtypeenum:str=None 

    # extends
    structextends:str=None
    nextStructs = None

    # platform
    platform:str=None

    # is pod
    bIsPod:bool=True

    #
    protect=''

    def addNextType(self, name, next):
        if self.nextStructs == None:
            self.nextStructs = {}
        self.nextStructs[name] = next

    def __init__(self, rootNode='./') -> None:
        self.name = GetNodeAttrib('name', rootNode)
        if self.name == '':
            self.name = GetNodeSubText(rootNode, "name")
 
        self.alias = GetNodeAttrib('alias', rootNode)

        self.sType = GetNodeAttrib('values', rootNode)

        self.structextends = GetNodeAttrib('structextends', rootNode)
        self.requires = GetNodeAttrib('requires', rootNode)

        self.parent = GetNodeAttrib('parent', rootNode) 
        self.objtypeenum = GetNodeAttrib('objtypeenum', rootNode)

        category = GetNodeAttrib('category', rootNode)
 
        match(category):
            case 'struct':
                self.category = ETypeCategory.Struct
            case 'enum':
                self.category = ETypeCategory.Enum
            case 'include':
                self.category = ETypeCategory.Include
            case 'handle':
                self.category = ETypeCategory.Handle
                # is VkInstance or alias or has parent
                if self.parent == None and self.name !='VkInstance' and self.alias=='':
                    raise ValueError(f'Vkhandle {self.name} should have parent ')
            case 'define':
                self.category = ETypeCategory.Define
            case 'basetype':
                self.category = ETypeCategory.BaseType
            case 'bitmask':
                self.category = ETypeCategory.Typedef
            case 'funcpointer':
                self.category = ETypeCategory.FunPtr
            case 'union':
                self.category = ETypeCategory.Union

        if self.requires!='':
            self.category = ETypeCategory.Require
        if self.name in ('int'):
            self.category = ETypeCategory.BaseType
        
        if self.category == ETypeCategory.Invalid :
            raise ValueError(f'parse type {self.name} failed because type not set')
        
        self.members = { member.name: member for member in [VkTypeMember(memberNode)  for memberNode in rootNode.findall('member')]}
 
        for paramName, param in self.members.items():
            if param.pointer!='' and paramName!='pNext':
                self.bIsPod = False
            if param.lengthParameters!=None and len(param.lengthParameters)>0:
                if param.lengthParameters[0] in self.members:
                    param.lengthMember = self.members[param.lengthParameters[0]]
                    param.lengthMember.bIsLength = True
                    if param.arrayCategory==EArrayType.Other:
                        raise ValueError(f"param {param.name} category error")
                if len(param.lengthParameters)>1:
                    if param.lengthParameters[1] !='null-terminated' and not param.lengthParameters[1].isdigit():
                        raise ValueError(f"param {param.name} category error")

        if 'sType' in self.members:
            
            member = self.members['sType'] 
            #print(member.values)
            if member.values !='VkStructureType':
                self.sType = member.values
        

class VkEnumValue:
    # type
    type:str=None
    name:str=None 
    value:str=None
    bBitMask:bool=False

    # alias name if exists, none if not exists
    alias:str=None 

    # comment string if exists, none if not exists
    comment:str=None
    
    # from feature or extend
    extends:str=None
    require:str=None

    # may has platform 
    protect:str=None
    # may has extension and feature not enabled
    enabled:bool=True

    def __str__(self) -> str:
        contentStr=''
        if self.type !=None:
            contentStr+='{type} '.format(type=self.type)
        contentStr+='{name} = '.format(name=self.name)
        if self.bBitMask:
            contentStr+='1 << {value}'.format(value=self.value)
        elif self.alias!=None:
            contentStr+='{alias}'.format(alias=self.alias)
        else:
            contentStr+='{value}'.format(value=self.value)
        return contentStr
        
    def __init__(self, rootNode, bExtend=False) -> None:
        self.alias = GetNodeAttrib('alias', rootNode)
        self.name = GetNodeAttrib('name', rootNode)

        if self.name =='':
            raise ValueError(f'enum value no name found')
        
        self.value = GetNodeAttrib('value', rootNode)
        bitpos = GetNodeAttrib('bitpos', rootNode)
        if bitpos!='':
            self.bBitMask = True
            self.value = bitpos

        if self.value == '' and self.alias == '' and not bExtend:
            raise TypeError('enum value {name} dont have value or bitpos attrib and is not alias'.format(name=self.name))
        
        self.comment = GetNodeAttrib('comment', rootNode)
        self.type = GetNodeAttrib('type', rootNode)
        self.extends = GetNodeAttrib('extends', rootNode)

class VkEnum:
    name:str=None
    type:EEnumCategory=EEnumCategory.Enum
    increaseMaxValue:VkEnumValue=None 
    comment:str=None
    values:dict[str,VkEnumValue]=None
    protect:str=''

    def __str__(self) -> str:
        enumStr=''

        if self.comment !=None:
            enumStr+='\n\\\\'+self.comment

        enumStr+='enum class'
        if self.type == EEnumCategory.Enum:
            enumStr+=' {name}:NormalEnum{{\n'.format(name=self.name)
        elif self.type == EEnumCategory.BitMask:
            enumStr+=' {name}:BitMask{{\n'.format(name=self.name)
        elif self.type == EEnumCategory.BitMask64:
            enumStr+=' {name}:BitMask64{{\n'.format(name=self.name)
        elif self.type == EEnumCategory.EnumExtend:
            enumStr+=' {name}:EnumExtend{{\n'.format(name=self.name)
        elif self.type == EEnumCategory.ValueCollections:
            enumStr+=f' {self.name}:Collection'
        else:
            raise ValueError("no enum type found")
        index=0
        for enumItem in self.values:
            if index!=0:
                enumStr+=', \n'
            index=index+1
            enumStr+=str(enumItem)
        enumStr+='};'
        return enumStr
 
    def __init__(self, rootNode) -> None:
        # name
        self.name = rootNode.attrib['name'].strip()

        # type
        if 'type' not in rootNode.attrib:
            self.type = EEnumCategory.ValueCollections
        else:
            typeStr = rootNode.attrib['type'].strip()
            if typeStr =='enum':
                self.type = EEnumCategory.Enum
            elif typeStr =='bitmask':
                self.type = EEnumCategory.BitMask
                if 'bitwidth' in rootNode.attrib:
                    bitwidth = rootNode.attrib['bitwidth'].strip()
                    if bitwidth =='64':
                        self.type = EEnumCategory.BitMask64
                    else:
                        raise TypeError('enum {name} bitwidth is exceptional'.format(**locals()))
            else:
                raise TypeError('enum {name} type is invalid'.format(**locals()))

        # enum comment
        commentNode = GetNodeSubText(rootNode, 'comment')

        # values
        self.values = { valueNode.attrib['name']:VkEnumValue(valueNode) for valueNode in rootNode.findall('enum') }

        # is mono-tone increasing
        if self.type == EEnumCategory.Enum:
            last:VkEnumValue=None
            bInceasingEnum = True
            for key,value in self.values.items():
                # alias
                if value.alias !=None:
                    continue 
                
                # first
                if last == None:
                    continue 
                 
                if value != last.value+1:
                    bInceasingEnum=False
                    break
                
                last = value
            
            if bInceasingEnum:
                self.type = EEnumCategory.EnumExtend
            
            self.increaseMaxValue = last

class VkFunctionity:
    name:str=None
    number:str=None 
    commands:list[str]=None 
    types:list[str]=None
    enums:list[VkEnumValue]=None 
    platform:str=None
    protect:str=None
    enabled:bool=True
    
    def __str__(self) -> str:
        return f'[{self.number}]={self.name}, platform={self.platform} commands={str(self.commands)}'

    def __init__(self, rootNode) -> None:
        self.name = GetNodeAttrib('name', rootNode)
        self.number = GetNodeAttrib('number', rootNode)
        self.platform = GetNodeAttrib('platform', rootNode)
        self.commands=[ command.attrib['name'].strip() for command in rootNode.findall('require/command') if 'name' in command.attrib ]
        self.types= [ command.attrib['name'].strip() for command in rootNode.findall('require/type') if 'name' in command.attrib ]
        self.enums= [ VkEnumValue(command, True) for command in rootNode.findall('require/enum') if 'name' in command.attrib ]


class VkFeature(VkFunctionity):
    api:str=None 
    comment:str=None 
    def __init__(self, rootNode) -> None:
        super().__init__(rootNode)
        self.api = GetNodeAttrib('api', rootNode)
        self.comment = GetNodeAttrib('comment', rootNode)

class VkExtension(VkFunctionity):
    # instance/device
    type:str=None
    version:str=''
    strName:str=''
    author:str=None 
    requires:str=None
    def __init__(self, rootNode) -> None:
        super().__init__(rootNode)
        self.type = GetNodeAttrib('type', rootNode)
        self.author = GetNodeAttrib('author', rootNode)
        self.requires = GetNodeAttrib('requires', rootNode)
        support = GetNodeAttrib('supported', rootNode)
        if support == 'disabled':
            self.enabled = False

        enums =[ GetNodeAttrib('name', enum) for enum in rootNode.findall('require/enum') if GetNodeAttrib('value', enum)!='' ]
        if len(enums)>=2:
            self.version = enums[0]
            self.strName = enums[1]


class VkHandle:
    name:str= ''
    type:VkType= None
    parentName = ''
    parent =None 
    children = None
    commands:dict[str,VkCommand] =None
    params:dict[str, int] =None

    commandstrs:dict[str,str]=None 
    commandDispatch:ECommandType = ECommandType.Instance

    def __init__(self, type:VkType) -> None:
        if type == None:
            raise ValueError("none type")
        self.type = type 
        self.name = type.name 
        self.parentName = type.parent
        self.children={}
        self.commands={}
        self.params={}
        self.commandstrs={}
        if self.name == 'VkInstance':
            self.commandDispatch = ECommandType.Instance
        elif self.name == 'VkDevice':
            self.commandDispatch = ECommandType.Device

    def setParent(self, parent):
        self.parent =parent 
        parent.children[self.name] = self

    def setTopMost(self):
        if self.parent == None:
            self.commandDispatch = ECommandType.Instance
            return

        topParent = self.parent
        while topParent.name!='VkInstance' and topParent.name!='VkDevice':
            topParent = topParent.parent
        self.commandDispatch = topParent.commandDispatch


class FormatComponent:
    name=''
    bits=''
    numericFormat=''

class FormatPlane:
    index=''
    widthDivisor=''
    heightDivisor=''
    compatible=''


class Format:
    name=''
    classStr=''
    blockSize=''
    texelsPerBlock=''
    packed=''
    sprivFormat=''
    blockExtent=''
    chroma=''
    compressed=''
    Planes:dict[str,FormatPlane]={}
    components:dict[str,FormatComponent]={}

    def __init__(self, rootNode) -> None:
        self.name = GetNodeAttrib( 'name', rootNode)
        self.classStr = GetNodeAttrib( 'class', rootNode)
        self.blockSize = GetNodeAttrib( 'blockSize', rootNode)
        self.texelsPerBlock = GetNodeAttrib( 'texelsPerBlock', rootNode)
        self.packed = GetNodeAttrib( 'packed', rootNode)
        self.blockExtent = GetNodeAttrib( 'blockExtent', rootNode)
        self.compressed = GetNodeAttrib( 'compressed', rootNode)

    def __str__(self) -> str:
        return f'format[ {self.name} ] = {{ class={self.classStr}, blockSize={self.blockSize}, texelsPerBlock={self.texelsPerBlock}, blockExtent={self.blockExtent}, compress={self.compressed} }}'

    def isCompressed(self)->bool:
        return self.compressed!=''


class VkXmlParser:
    # script execute path
    scriptPath:str=None
    # parsed tree
    tree =None
    # declare
    declare:str =None

    #platform: name => protect
    platforms:dict[str,str]=None

    # tag
    tags:list[str]=None

    # type
    typelist:list[VkType]=None
    types:dict[str,VkType]=None
    structTypes:dict[str,VkType]=None

    handles: dict[str, VkHandle] =None

    # enum name => content
    enums:dict[str,VkEnum]=None 
    constantEnums:dict[str,VkEnumValue]=None

    # vk function
    commandDefines:dict[str,VkCommand]=None

    #features
    features:list[VkFeature]=None

    # extension
    extensions:list[VkExtension]=None

    # extension + features
    functionalities:list[VkFunctionity]=None

    # include
    includeExtensions:set[str]=None

    # format
    formats:dict[str,Format]=None

    # exclude
    excludeCommands:set[str]={}
    excludeTypes:set[str]={}
    excludeEnums:set[str]={}

    #context 
    protect=None
    isExtension = False
    commandDup={}
    typeDup={}
    enumDup={}

    bPrint = False

    def setIncludeExtensions(self, exts:set[str]):
        self.includeExtensions = exts

    def setExcludeTypeAndCommand(self, excludeCommands:set[str] ={}, excludeEnums:set[str]={}, excludeTypes:set[str]={} ):
        self.excludeCommands = excludeCommands
        self.excludeEnums = excludeEnums
        self.excludeTypes = excludeTypes

    def getTypeByName(self, name:str):
        if name in self.types: 
            return self.types[name]
        if name in self.enums:
            return self.enums[name]
        return None
 
    def __init__(self, treePath='./') -> None:
        ''' 
            create the xml parse tree from treePath,then parse into TypeDef/CommandDef
        '''

        # create parser
        self.scriptPath = os.path.abspath(os.path.dirname(sys.argv[0])) + os.path.sep
        instance = XmlTree.parse(self.scriptPath + treePath + 'vk.xml')
        if instance == None:
            raise ValueError("vk.xml don't exists in path:%s".format(treePath))
        
        # tree root
        tree = instance.getroot()
        self.tree = tree 

        # get declare comment
        declare=''
        for comment in tree.findall('comment')[0:2]:
            declare += "\n* ".join( [line.strip() for line in comment.text.split('\n')] )
        self.declare=declare

        if self.bPrint:
            print("===== declare:\n", self.declare)

        # platforms
        self.platforms={ plat.attrib['name']: plat.attrib['protect'] for plat in tree.findall('platforms/platform') if 'name' in plat.attrib }
        if self.bPrint:
            print("===== platform:\n",self.platforms)

        # type
        self.typelist = [VkType(typeNode) for typeNode in tree.findall('types/type')]
        self.types ={ type.name: type for type in self.typelist }

        # struct
        self.structTypes = {  name:type for name, type in self.types.items() if type.category == ETypeCategory.Struct}

        # find all struct extends
        for structTypeName, structType in self.structTypes.items():
            if structType.structextends!='':
                extends = structType.structextends.split(',')
                if len(extends) > 0:
                    for extend in extends:
                        if extend in self.structTypes:
                            extendStruct = self.structTypes[extend]
                            if extendStruct !=None:
                                extendStruct.addNextType(structTypeName, structType)
                                print(f"extends {structTypeName} has next {extend} ")
                            else:
                                raise ValueError(f"{extend} extends but not found type")
                        else:
                            raise ValueError(f"{extend} extends but not found type")

        # handle 
        self.handles = { type.name : VkHandle(type) for type in self.typelist if type.category == ETypeCategory.Handle }

        # link handle tree
        map = {}
        for name,handle in self.handles.items():
            if handle.parentName!='':
                if  handle.parentName not in self.handles:
                    raise ValueError(f"handle {name} parent {handle.parentName} not found in handles")
                else:
                    handle.setParent(self.handles[handle.parentName])
 
        for name,handle in self.handles.items():
            handle.setTopMost()

        # command defines
        commandDefines={}
        for commandNode in tree.findall('commands/command'):
            # typdef 
            if 'alias' in commandNode.attrib:
                name = commandNode.attrib['name']
                alias = commandNode.attrib['alias']
                if alias not in commandDefines:
                    raise ValueError('alias {} of {} defined, but {} is unknown'.format(name, alias, alias))
                commandDefines[name] = commandDefines[alias]
                continue
            
            # already added
            name = commandNode.find('proto/name').text
            if name in commandDefines:
                continue

            # add new command define
            commandDefines[name] = VkCommand(name, commandNode)
        self.commandDefines = commandDefines
        #print(tuple(self.commandDefines))

        if self.bPrint:
            for key,value in self.commandDefines.items():
                pass #print("{key}: {value}\n".format(**locals()))

        
        self.formats = { format.name:format for format in  ( Format(formatNode) for formatNode in tree.findall('formats/format')) }
       


        
        # link then handle and command, then first parameter is handle as member
        for name, command in self.commandDefines.items():
            index =0
            if command.commandType != ECommandType.Global:
                for paramName, param in command.params.items():
                    if param.type in self.handles and index == 0:
                        handle = self.handles[param.type]
                        handle.commands[name] = command
                        handle.commandstrs[name] = command.getDeclare()[0]
                        handle.params[paramName] = index
                    index = index+1
        
        # enums
        self.enums={ enum.attrib['name'].strip(): VkEnum(enum)  for enum in tree.findall('enums') if 'name' in enum.attrib }

        if self.bPrint:
            for key,value in self.enums.items():
                pass #print('{key}: {value}'.format(**locals()))

        # tag
        self.tags ={ tagNode.attrib['name'] for tagNode in tree.findall('tags/tag') if 'name' in tagNode.attrib }

        if self.bPrint:
            print("tags:", self.tags)

        # extension

        self.extensions = [  availableExtension for availableExtension in ( VkExtension(extension) for extension in tree.findall('extensions/extension')) if availableExtension.enabled]
        self.features= [  VkFeature(feature) for feature in tree.findall('feature') ]
        self.functionalities = self.features + self.extensions


        def extSort(ext:VkExtension):
            if 'KHR' in ext.name:
                return int(ext.number)
            return 10000000 + int(ext.number)

        self.extensions = sorted(self.extensions, key=extSort)
 
        # setting 
        for functionality in self.functionalities:
            print("## functionality:", functionality.name)
            platform =''
            if functionality.platform!='':
                functionality.protect = self.platforms[ functionality.platform]

            # mark enum's platform
            for enumExt in functionality.enums:
                if enumExt.extends !='':
                    if enumExt.extends not in self.enums:
                        raise ValueError(f'enum extends {enumExt.extends} not found in enum define')
                    enumExt.require = functionality.name

                    enum = self.enums[enumExt.extends]
                    enum.values[enumExt.name] = enumExt
                    enumExt.protect = functionality.protect

        
        
        for functionality in self.functionalities:
            if functionality:   
                for commandName in functionality.commands:
                    if commandName in self.commandDefines:
                        command = self.commandDefines[commandName]
                        if command:
                            command.protect = functionality.protect

                for enumName in functionality.enums:
                    if enumName in self.enums:
                        enum = self.enums[enumName]
                        if enum:
                            enum.protect = functionality.protect
                
                for typeName in functionality.types:
                    if typeName in self.types:
                        type = self.types[typeName]
                        if type:
                            type.protect = functionality.protect

    
    def foreachTypeAndCommandInFunctionity(self, functionality:VkFunctionity=None, commandCallback:Callable = None, enumCallback:Callable= None, typeCallback:Callable= None):
        # command exists and not exclude

        if commandCallback:
            for commandName in functionality.commands:
                if commandName in self.commandDefines and commandName not in self.excludeCommands:
                    if commandName not in self.commandDup:
                        self.commandDup[commandName] = 1
                        command = self.commandDefines[commandName]
                        if command:
                            commandCallback(self, commandName, command)

        if enumCallback:
            for enumName in functionality.types:
                if enumName in self.enums and enumName not in self.excludeEnums:
                    enum = self.enums[enumName]
                    if enum and enumName not in self.enumDup:
                        self.enumDup[enumName] =1
                        enumCallback(self, enumName, enum)

        for typeName in functionality.types:
            if typeName in self.types and typeName not in self.excludeTypes:
                type = self.types[typeName]
                if type and typeName not in self.typeDup:
                    self.typeDup[typeName] =1
                    if typeCallback:
                        typeCallback(self, typeName, type)

    def foreachFunctionity(self, functionalityCallback:Callable =None, commandCallback:Callable = None, enumCallback:Callable= None, typeCallback:Callable= None):
        if not functionalityCallback and not commandCallback and not enumCallback and not typeCallback:
            raise ValueError(f'a callback should be set to process functionity/enum/type/command')

        self.commandDup={}
        self.enumDup={}
        self.typeDup={}

        dependsExtension = deque()
        removedup ={} 
        for functionality in self.functionalities:
            if functionality:
                if not functionality.enabled:
                    print(f'extension {functionality.name} not enabled')
                    continue
                elif functionality.name in includeExtensions:
                    self.protect = functionality.protect
                    self.isExtension = isinstance(functionality, VkExtension) 
                    self.foreachTypeAndCommandInFunctionity(functionality, commandCallback, enumCallback, typeCallback)
                    functionalityCallback(self, functionality.name, functionality)
                    removedup[functionality.name]=1
                    if isinstance( functionality, VkExtension):
                        if functionality.requires!='':
                            requires = functionality.requires.split(',')
                            for req in requires:
                                if req not in removedup:
                                    dependsExtension.append(req)
                                    removedup[req]=1

        # depends extension
        extensionMap = {extension.name:extension for extension in self.extensions}
        while len(dependsExtension) >0:
            dependName = dependsExtension.pop()
            
            if dependName in extensionMap:
                func = extensionMap[dependName]
                self.foreachTypeAndCommandInFunctionity(func, commandCallback, enumCallback, typeCallback)
                functionalityCallback(self, dependName, func)
                if func.requires!='':
                    requires = func.requires.split(',')
                    for req in requires:
                        if req not in removedup:
                            dependsExtension.append(req)
                            removedup[req]=1

    def writeDeclare(self, file:io.TextIOWrapper, bHeader=True):
        filename = __file__
        filename = filename.replace('\\','/')
        scriptFile = filename[ filename.rfind('/')+1:len(filename)]
        #print(scriptFile)

        Pragma='#pragma once'
        if not bHeader:
            Pragma =''

        file.write('''
/******************************************************************************
* The MIT License (MIT)
*
* Copyright (c) 2020-2021 Baldur Karlsson
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
******************************************************************************/

/******************************************************************************
* The MIT License (MIT)
*
* Copyright (c) 2022-2023 xiongya
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
******************************************************************************/

/******************************************************************************
* Generated from Khronos's vk.xml:
{vkComments}

******************************************************************************/
// This file is autogenerated with {scriptName} - any changes will be overwritten next time
// that script is run.
// $ ./{scriptName}

{Pragma}

// this file is autogenerated, so don't worry about clang-format issues
// clang-format off
        '''.format(scriptName=scriptFile, vkComments=self.declare, Pragma = Pragma).strip())


parser = VkXmlParser('./')
scriptPath = os.path.abspath(os.path.dirname(sys.argv[0])) + os.path.sep


# need get api addr but no need implement auto
exclude_impl_apis={
   'vkCreateInstance', 
   'vkCreateDevice',
   'vkGetInstanceProcAddr',
   'vkGetDeviceProcAddr',
   'vkEnumerateInstanceLayerProperties',
   'vkEnumerateDeviceLayerProperties',
   'vkEnumerateInstanceExtensionProperties',
   'vkEnumerateDeviceLayerProperties',
   'vkEnumerateDeviceExtensionProperties'
}

excludeCommands={
    'vkEnumerateInstanceVersion'
}

includeExtensions={
    'VK_VERSION_1_0',
    'VK_VERSION_1_1',
    'VK_VERSION_1_2',
    'VK_EXT_debug_marker',
    'VK_KHR_swapchain', 
    'VK_KHR_surface',
    'VK_KHR_display',
    "VK_KHR_android_surface", # android window surface
    "VK_KHR_win32_surface",   # window
     
    'VK_EXT_swapchain_colorspace', #instance
    'VK_EXT_validation_features', #instance
    'VK_KHR_device_group_creation', #instance
    'VK_KHR_external_fence_capabilities', #instance
    'VK_KHR_external_memory_capabilities', #instance
    'VK_KHR_external_semaphore_capabilities', #instance
    'VK_KHR_get_physical_device_properties2', #instance
    'VK_KHR_get_surface_capabilities2', #instance

    #'VK_ANDROID_external_memory_android_hardware_buffer', #device
    'VK_EXT_astc_decode_mode', #device
    'VK_EXT_filter_cubic', #device
    'VK_EXT_fragment_density_map', #device
    'VK_EXT_global_priority', #device
    'VK_EXT_hdr_metadata', #device
    'VK_EXT_line_rasterization', #device
    'VK_EXT_pipeline_creation_feedback', #device
    'VK_EXT_queue_family_foreign', #device
    'VK_EXT_sample_locations', #device
    'VK_EXT_sampler_filter_minmax', #device
    'VK_EXT_scalar_block_layout', #device
    'VK_EXT_transform_feedback', #device
    'VK_GOOGLE_display_timing', #device
    'VK_IMG_filter_cubic', #device
    'VK_KHR_16bit_storage', #device
    'VK_KHR_bind_memory2', #device
    'VK_KHR_create_renderpass2', #device
    'VK_KHR_dedicated_allocation', #device
    'VK_KHR_depth_stencil_resolve', #device
    'VK_KHR_descriptor_update_template', #device
    'VK_KHR_device_group', #device
    'VK_KHR_draw_indirect_count', #device
    'VK_KHR_driver_properties', #device
    'VK_KHR_external_fence', #device
    'VK_KHR_external_fence_fd', #device
    'VK_KHR_external_memory', #device
    'VK_KHR_external_memory_fd', #device
    'VK_KHR_external_semaphore', #device
    'VK_KHR_external_semaphore_fd', #device
    'VK_KHR_get_memory_requirements2', #device
    'VK_KHR_image_format_list', #device
    'VK_KHR_incremental_present', #device
    'VK_KHR_maintenance1', #device
    'VK_KHR_maintenance2', #device
    'VK_KHR_maintenance3', #device
    'VK_KHR_multiview', #device
    'VK_KHR_push_descriptor', #device
    'VK_KHR_relaxed_block_layout', #device
    'VK_KHR_sampler_mirror_clamp_to_edge', #device
    'VK_KHR_sampler_ycbcr_conversion', #device
    'VK_KHR_shader_draw_parameters', #device
    'VK_KHR_shader_float16_int8', #device
    'VK_KHR_shader_float_controls', #device
    'VK_KHR_shared_presentable_image', #device
    'VK_KHR_storage_buffer_storage_class', #device
    'VK_KHR_swapchain', #device
    'VK_KHR_variable_pointers', #device
    'VK_KHR_vulkan_memory_model', #device
    'VK_QCOM_render_pass_transform', #device
    'VK_KHR_fragment_shading_rate',
    "VK_EXT_validation_cache",
    
}


    

# which 
parser.setIncludeExtensions(includeExtensions)

"""
write vk_dispatch_table.h
"""
with open(scriptPath + 'vk_dispatch_defs.h', mode='w', newline = None) as dispatchTableFile:
    parser.writeDeclare(dispatchTableFile)

    AllContents=['','','','']
    FunctionalContents=['','','','']


    def ProcessEachCommand( parser:VkXmlParser, name:str, command:VkCommand):
        global FunctionalContents,GlobalCommands

        if command != None:
            if command.commandType == ECommandType.Global:
                FunctionalContents[2] +=f"\n\tPFN_{name} {name};"
                FunctionalContents[3] +=f"\\\n\t\tEnumMacro({name})"
            elif command.commandType == ECommandType.Device:
                FunctionalContents[0] +=f"\n\tPFN_{name} {name};"
            elif command.commandType == ECommandType.Instance:
                FunctionalContents[1] +=f"\n\tPFN_{name} {name};"

    def ProcessEachFunctionality(parser:VkXmlParser, name:str, functionality:VkFunctionity):
        global FunctionalContents, AllContents

        platform = functionality.platform
        for i in range(4):
            if functionality.platform!='':
                platformName = parser.platforms[ platform ]
                if  FunctionalContents[i] != "":
                    FunctionalContents[i] = f'\n#ifdef {platformName}{ FunctionalContents[i]}\n#endif // {platformName}'
        
            if FunctionalContents[i] !='' and i!=3:
                 FunctionalContents[i]  = f"// {name}{FunctionalContents[i] }\n\n"

            AllContents[i] +=FunctionalContents[i]
            FunctionalContents[i]=''
        


    parser.foreachFunctionity(ProcessEachFunctionality, commandCallback=ProcessEachCommand)

    dispatchTableFile.write(
f'''

struct VkGlobalDispatchTable
{{
    {AllContents[2]}
}};

struct VkDeviceCommands
{{
  {AllContents[0]}
}};

struct VkDevDispatchTable: VkDeviceCommands{{
    VkDevice handle;
    VkPhysicalDevice physicalHandle;
}};

struct VkInstDispatchTable: VkDeviceCommands
{{
    VkInstance handle;
  {AllContents[1]}
}};
''')


"""
write vk_api_enumerators.h
"""
with open(scriptPath + 'vk_api_enumerators.h', mode='w', newline = None) as CommandInfo:
    parser.writeDeclare(CommandInfo)

    # each is  DeviceAddr, InstanceAddr, DeviceLocalExport, InstanceLocalExport
    MacroEnumators=['','','','']
    GlobaEnumators=''
    # 
    SectionEnumators=['','','','']
    FunctionalitySectionEnumators=['','','','']

    # this function is implements mannally
    excludeLocalCommands={
        'vkCreateInstance', 
        'vkCreateDevice',
        'vkGetInstanceProcAddr',
        'vkGetDeviceProcAddr',
        'vkEnumerateInstanceLayerProperties',
        'vkEnumerateDeviceLayerProperties',
        'vkEnumerateInstanceExtensionProperties',
        'vkEnumerateDeviceExtensionProperties'
    }

    def ProcessEachCommand( parser:VkXmlParser, name:str, command:VkCommand):
        global excludeLocalCommands,FunctionalitySectionEnumators,GlobaEnumators
        
        decl =  f'\\\n\t\t\tMacro({name})'
        impl =  f'\\\n\t\t\tMacro({name})'

        if command.commandType == ECommandType.Global:
            GlobaEnumators +=decl
        elif command.commandType == ECommandType.Instance:
            FunctionalitySectionEnumators[0] +=decl
            if name not in excludeLocalCommands:
                FunctionalitySectionEnumators[1] +=impl
        elif  command.commandType == ECommandType.Device:
            FunctionalitySectionEnumators[2] +=decl
            if name not in excludeLocalCommands:
                FunctionalitySectionEnumators[3] +=impl
        else:
            raise ValueError(f'command {name} has unknown table type {str(command.commandType)}')

    def ProcessEachFunctionality(parser:VkXmlParser, name:str, functionality:VkFunctionity):
        global FunctionalitySectionEnumators

        upperName = name.replace(' ','_').upper()
        typeNames = ('INSTANCE', 'DEVICE', 'INSTANCE_IMPLEMENT', 'DEVICE_IMPLEMENT')

        protect = functionality.protect
        for i in range(4):
           
            if FunctionalitySectionEnumators[i]!='':
                macroName = f'ENUMERATE_{typeNames[i]}{upperName}(Macro)'
                emptyDefineMacro = f'\n#define {macroName}'
                defineMacro = f'\n#define {macroName}{FunctionalitySectionEnumators[i]}'

                sectionContent =''
                if protect!=None:
                    sectionContent= f'#ifdef {protect}{defineMacro}\n#else {emptyDefineMacro}\n#endif // {protect}'
                else :
                    sectionContent = defineMacro
                 
                # content accumulate
                MacroEnumators[i] +=f'\\\n\t\t{macroName}'
                SectionEnumators[i]+= f"//  {name}\n{sectionContent}\n\n"
                FunctionalitySectionEnumators[i]=''

    parser.foreachFunctionity(ProcessEachFunctionality, commandCallback=ProcessEachCommand)

    CommandInfo.write(
f'''
#define ENUM_VK_ENTRYPOINTS_BASE(Macro){GlobaEnumators}

// Instance enumerate
#define ENUMERATE_VK_INSTANCE_API(Macro){MacroEnumators[0]}
// Device enumerate
#define ENUMERATE_VK_DEVICE_API(Macro){MacroEnumators[2]}

//Instance api local implements
#define ENUMERATE_VK_INSTANCE_API_LOCAL_IMPLEMENT(Macro){MacroEnumators[1]}
//Instance api local implements
#define ENUMERATE_VK_DEVICE_API_LOCAL_IMPLEMENT(Macro){MacroEnumators[3]}

//Instance api
{SectionEnumators[0]}

// Device api
{SectionEnumators[1]}

//Instance api local implements
{SectionEnumators[2]}

// Device api local implements
{SectionEnumators[3]}
''')


"""
write vk_enum_defs.h
"""
with open(scriptPath + 'vk_enum_defs.h', mode='w', newline = None) as CommandInfo:
    parser.writeDeclare(CommandInfo)

    SectionContents=''
    AllContents=''

    def ProcessEachEnum( parser:VkXmlParser, name:str, enum:VkEnum):
        global SectionContents

        if name=='API Constants':
            return

        if len(enum.values) == 0:
            return

        define=''

        # define Get%Enum%String function
        define+=f'\ninline char const* Get{name}String( {name} value){{\n\tswitch(value){{'

        bHasElement = False
        
        for valueName, value in enum.values.items():
            if value.alias!='':
                continue
            bHasElement=True 
            case = WrapProtect(value.protect, f'\n\t\tVkEnumNameCase({valueName})');
            define+=case
        
        # no element, filter out
        if not bHasElement:
            return

        define+=f'\n\t\tdefault: (void)0;\n\t\t}}\n\treturn "[Unkown]";\n}}'

        '''
        def TransformStr(name:str, parser:VkXmlParser, CommonLength:int):
            lastPos= 0
            currentPos=0
            newName=''
            for c in name:
                currentPos=currentPos+1
                if c =='_':
                    newName+= name[lastPos:currentPos-1].capitalize()
                    lastPos=currentPos
            if name[lastPos:] in parser.tags:
                newName+=name[lastPos:]    
            else:
                newName+=name[lastPos:].capitalize()
            
            newName=newName[CommonLength:]

            if len(newName)>0:
                first = newName[0]
                if first.isdigit():
                    newName='_'+newName
            return newName

        if enum.type == EEnumCategory.BitMask or enum.type == EEnumCategory.BitMask64:
            typeStr='uint32_t'
            bitCountStr='32'
            if enum.type == EEnumCategory.BitMask64:
                typeStr='uint64_t'
                bitCountStr='64'
            
            define+=f'\n\ninline void GetAll{name}String( {name} value, std::string& str, char split=\',\' ){{\n\t for({typeStr} i=0,e=(i<<1); i<{bitCountStr};++i)\n\t{{'\
                f'\n\t\t if( value & e) str+=Get{name}String(({name})e); str.push_back(split); \n\t}}\n}}'
        elif enum.type == EEnumCategory.EnumExtend:
            transformEnum =f'\nenum class ETransform{name}{{'
            transformFunc =f'\ninline {name} GetTransform2{name}( ETransform{name} value ){{\n\tswitch(value){{'
            transformFromFunc=f'\ninline ETransform{name} Get{name}2Transform( {name} value ){{\n\tswitch(value){{'
            enumPrefixLength = len(name)

            for tag in parser.tags:
                if name.endswith(tag):
                    enumPrefixLength -=len(tag)

            if name=='VkResult':
                enumPrefixLength=2

            index=0
            for valueName, value in enum.values.items():
                if value.require!=None and value.require!='' and value.require not in includeExtensions:
                    continue 
                if value.alias!='':
                    continue
                newName =  TransformStr(value.name, parser,enumPrefixLength)

                transformEnum+=f'\n\t{newName} = {str(index)},'
                index=index+1
                transformFunc+=f'\n\t\tcase ETransform{name}::{newName}: return {valueName};'
                transformFromFunc+=f'\n\t\tcase {valueName}: return ETransform{name}::{newName};'
                if index!=0:
                    transformEnum+=f'\n\tCount = {str(index)}\n}};\n'
                    transformFunc+=f'\n\t\tdefault: (void)0;\n\t}}\n\treturn ({name})0; \n}}\n'
                    transformFromFunc+=f'\n\t\tdefault: (void)0;\n\t}}\n\treturn (ETransform{name})0; \n}}\n'
                    define+=transformEnum
                    define+=transformFunc
                    define+=transformFromFunc
        

            define+=f'\ninline char const* GetTransform{name}String( ETransform{name} value ){{\n return Get{name}String(GetTransform2{name}(value));\n}}'
        '''
        SectionContents +=define 

    def processEachFunctionlity(parser:VkXmlParser, name:str, functionality:VkFunctionity):
        global SectionContents,AllContents
        protect = functionality.protect
        if SectionContents!='':
            AllContents+= WrapProtect(protect, SectionContents)
            SectionContents=''

    parser.foreachFunctionity(processEachFunctionlity, enumCallback=ProcessEachEnum)

    CommandInfo.write(
f'''
#define VkEnumNameCase(Enum) case Enum: return #Enum;
{AllContents}
#undef VkEnumNameCase
''')



"""
write vk_command_defs.h
"""
with open(scriptPath + 'vk_command_defs.h', mode='w', newline = None) as CommandInfo:
    parser.writeDeclare(CommandInfo)

    SectionContents=['','']
    AllContents=['','']

    def ProcessEachCommand( parser:VkXmlParser, name:str, command:VkCommand):
        global SectionContents

        declTuple = command.getDeclare()
        oneDefine =  f'\n#define {name}_define \\\n     ({name}, {command.ret}, ({declTuple[0]}),\
                \\\n     ({name}, {command.ret}, {declTuple[1]}),\\\n     ({name}, {command.ret}, {declTuple[2]}) )\n\n'
        if command.commandType == ECommandType.Instance or command.commandType == ECommandType.Global:
            SectionContents[0] +=oneDefine
        elif  command.commandType == ECommandType.Device:
            SectionContents[1] +=oneDefine
        else:
            raise ValueError('command {} has unknown table type {}'.format(name, command.commandType))

    def ProcessEachFunctionality(parser:VkXmlParser, name:str, func:VkFunctionity):
        global AllContents,SectionContents

        protect = func.protect
        for i in range(2):
            if SectionContents[i]!='':
                if protect!=None:
                    SectionContents[i] = f'\n#ifdef {protect}{SectionContents[i]}\n#endif // {protect}'
                AllContents[i] += f"\t// {name}{SectionContents[i]}\n\n"
                SectionContents[i]=''

    parser.foreachFunctionity(ProcessEachFunctionality, commandCallback=ProcessEachCommand)

    CommandInfo.write(
f'''
//Instance api
{AllContents[0]}

// Device api
{AllContents[1]}

''')


"""
write vk_serialize_defs.h
"""
with open(scriptPath + 'vk_serialize_defs.h', mode='w', newline = None) as CommandInfoHeader:
    with open(scriptPath + 'vk_serialize_defs.cpp', mode='w', newline = None) as CommandInfoCPP:

        parser.writeDeclare(CommandInfoHeader)
        parser.writeDeclare(CommandInfoCPP,False)

        SectionContentsHeader=''
        SectionContentsCPP=''
        AllContentsHeader=''
        AllContentsCPP=''

        SectionCommandEnumlator=''
        AllCommandEnumlator=''

        SectionFuncDefine=''
        AllFuncDefine=''

        StructsDepends:dict[str,str]={}
        MappingNames:dict[str,str]={}

        def GetStructName(name:str):
            newStructName = name
            #for prefix in ('Vk', 'vk'):
            if name.startswith('Vk'):
                newStructName = name.removeprefix('Vk')
                newStructName = f'S{newStructName}'
            elif name.startswith('vk'):
                newStructName = name.removeprefix('vk')
                newStructName = f'S{newStructName}'
            return newStructName

        def GetNewStructName(name:str, protect:str=None):
            global MappingNames
            newStructName = GetStructName(name)
            MappingNames[name]=newStructName
            return newStructName
        
        def GetDependsStructName(name:str, protect:str=None):
            global MappingNames,StructsDepends
            if name in MappingNames:
                return MappingNames[name]
            
            structName= GetStructName(name)
            StructsDepends[name] = structName
            return structName

        # vkcommand
        def GenerateCommandStruct(parser:VkXmlParser, name:str, command:VkCommand):
            global SectionContentsHeader,SectionCommandEnumlator,GlobalCommands

            structName = GetNewStructName(name)
 
            parentName ='CommandInfo'
            parentDerive=f':public {parentName}'
            parentInitalize=f': {parentName}{{}}'
            parentDeepCopyInitalize=f': {parentName}{{kDeepCopy}}'

            sCommandType ='other'
            if name.startswith('vkCreate') or name.startswith('vkAllocate') or name in ('vkGetSwapchainImagesKHR'):
                sCommandType='create'
            elif name.startswith('vkDestroy') or name.startswith('vkFree'):
                sCommandType='destroy'

                

            commandTypeExpr=f'\n\tstatic constexpr EVkCommandType kCommandType=EVkCommandType::{sCommandType};'

            memberDeclExprList=''
            memberConstructorExprList=''
            constructorParamExprList=''
            deepCopyConstructorParamExprList=''

            
            toStringExprList=''
            serializeExprList=''
            deserializeExprList=''
            commandExecuteParamExprList=''
            commandBeforeExecuteExprList=''
            commandAfterExecuteExprList=''
            memberPostInitializeInConstructorList=''
            memberPostInitializeInDeepCopyConstructorList=''
            deepDestroyExprList=''
            memberDeepCopyConstructorExprList=''

            lengthCheck={}
            firstParamTypeName =''
            handleProcess='VkHandleProcess'

            for param,index in zip(command.params.values(), range(len(command.params))): 
                # member name, remve all p and lower then first char
                paramName=param.name
                paramType=param.type 
                fulltype = param.getFullType()
                
                # allocator
                bIsAllocator=False
                if paramName == 'pAllocator':
                    bIsAllocator=True
                    
                
                # add , except first param
                dot=''
                if index!=0:
                    dot=', '
                else:
                    firstParamTypeName = paramType
                
                # is pointer type
                bIsPointer = False 
                if param.pointer!='':
                    bIsPointer = True
                
                # param is another another type in types
                memberType = paramType
                bIsStruct = False
                bIsTypeStruct = False
                if param.type!=None and param.type in parser.types:
                    tryMemberType = parser.types[param.type ]
                    if tryMemberType.category == ETypeCategory.Struct :
                        #and not tryMemberType.bIsPod
                        memberType = GetDependsStructName(tryMemberType.name)
                        bIsStruct=True
                        if tryMemberType.sType!='':
                            bIsTypeStruct=True
                            
                        
                memberName = paramName
                # declare member
                memberDeclExpr=''
                # member constructor
                memberConstructorExpr=''
                # deep copy
                memberDeepCopyConstructorExpr=''
                memberPostInitializeInDeepCopyConstructor=''
                # member initialize in constructor
                memberPostInitializeInConstructor=''
                # toString function
                toStringExpr=''
                # command execute param
                commandExecuteParamExpr=''
                # before execute do some transform
                commandBeforeExecuteExpr=''
                # after execute do some transform
                commandAfterExecuteExpr=''
                # recycle resource
                deepDestroyExpr=''
                # 
                exprBeforeSerialize=''
                serializeParams=''
                deserializeParam=''
                exprBeforeDeserialize=''

                def AddLengthBeforeCommand(Name:str, Expr:str):
                    if Name in lengthCheck or Name.find('->')>=0:
                        return ''
                    lengthCheck[Name]=True
                    return Expr

                def GetRemovePointerName(name:str):
                    newName=name
                    if name.startswith('p'):
                        newName=newName.removeprefix('p')
                        newName=newName.removeprefix('p')
                        newName=newName.removeprefix('p')
                        if newName[0:1].isupper():
                            newName = newName[:1].lower() + newName[1:]
                    
                    return newName
                
                deserializeParam = serializeParams = memberName
                memberDeepCopyConstructorExpr=f'{memberName}{{{paramName} }}'

                if param.arrayCategory == EArrayType.Other:
                    # length, length pointer, void** buffer, normal param,  T* => length, *length, *buffer, *buffer+
                    if bIsPointer:
                        if not paramName.startswith('p'):
                            #raise ValueError('pointer type not start with p')
                            print(f"####### {command.name} param {paramName} pointer type not start with p")
                        
                        # remove p
                        memberName = GetRemovePointerName(paramName)

                        # is a buffer, special case
                        if param.pointer=='**':
                            if name == 'vkMapMemory':
      
                                memberDeclExpr=f'uint8_t* {memberName}; /* void* + size */\n\t'
                                memberDeclExpr+=f'uint8_t* mappedData; /* void* + size */\n\t'
                                memberDeclExpr+=f'uint8_t** {memberName}Ptr; /*the original ptr */\n\t'
                                memberDeclExpr+=f'uint64_t {memberName}Length;'

                                memberConstructorExpr=f'{memberName}{{ nullptr }}' 
                                memberConstructorExpr+=f'\n\t\t, mappedData{{ nullptr }}' 
                                memberConstructorExpr+=f'\n\t\t, {memberName}Ptr{{ (uint8_t**){paramName} }}' 
                               
                                lengthExpr =f'\n\t\t, {memberName}Length{{ size }}' 
                                #ptr::safe_new_default_array<uint8>(size)
                                memberDeepCopyConstructorExpr=f'{memberName}{{ nullptr }}'
                                memberDeepCopyConstructorExpr+=f'\n\t\t, mappedData{{ nullptr }}' 
                                memberDeepCopyConstructorExpr+=f'\n\t\t, {memberName}Ptr{{ (uint8_t**){paramName} }}'
                                memberConstructorExpr+=lengthExpr
                                memberDeepCopyConstructorExpr+=lengthExpr

                                toStringExpr = f'RS_TO_STRING({memberName});'
                                toStringExpr += f'\n\t\tRS_TO_STRING({memberName}Length);'
                                
                                # size should serialize before this
                                # serialize
                                deserializeParam = serializeParams = f'{memberName}, {memberName}Length, kReadWriteLength_Allocate'

                                # destroy
                                deepDestroyExpr = f'ptr::safe_delete_array({memberName}, {memberName}Length);'

                                # get data
                                #commandBeforeExecuteExpr=f'void* {memberName}Addr = nullptr;'
                                commandExecuteParamExpr=f'(void**)&mappedData'
                                commandAfterExecuteExpr=f'ARAssert(mappedData!=nullptr);\n\t\tif({memberName}Length>0){{ memcpy( mappedData, {memberName}, {memberName}Length);}}'
                        
                        elif paramType == 'void':
                            # void* with unkown pointer size, set a max size 2048 and size info is in content
                            memberDeclExpr=f'uint8_t* {memberName}; /* void* + size */\n\t' 
                            memberDeclExpr+=f'uint64_t {memberName}Length;' 
                            memberConstructorExpr=f'{memberName}{{  (uint8_t*){paramName} }}' 
                            memberDeepCopyConstructorExpr = f'{memberName}{{ ptr::safe_new_copy_array((uint8_t*){paramName}, 2048) }}'
                            lengthExpr =f'\n\t\t, {memberName}Length{{2048}}' 

                            memberConstructorExpr+=lengthExpr
                            memberDeepCopyConstructorExpr+=lengthExpr 

                             # serialize
                            deserializeParam = serializeParams = f'{memberName}, {memberName}Length, kReadWriteLength_Allocate'
                             # destroy
                            deepDestroyExpr = f'ptr::safe_delete_array({memberName}, {memberName}Length);'

                            toStringExpr = f'RS_TO_STRING({memberName});'
                            toStringExpr += f'\n\t\tRS_TO_STRING({memberName}Length);'


                            # command param
                            commandExecuteParamExpr=f'{memberName}'

                        elif paramName != 'pAllocator':
                            # pointer but not array, normal type
                            memberDeclExpr=f'{memberType}* {memberName}; /* T* */' 
                            

                            if param.bIsLength:
                                commandExecuteParamExpr=f'&{memberName}'
                            else:
                                commandExecuteParamExpr=f'{memberName}'

                            toStringExpr = f'RS_TO_STRING({memberName});'


                            deserializeParam = serializeParams = f'{memberName}, kReadWriteOptional'

                            inParam = paramName
                            if bIsStruct:
                                inParam=f'castDerived<{memberType}>({paramName})'
                                commandBeforeExecuteExpr=f'{memberName}->processHandle(handleReplace);'
                                #toStringExpr = f'RS_TO_STRING(castDerived<{memberType}>({paramName}));'
                            elif param.prefix.find('const')!=-1 :
                                inParam=f'const_cast<{memberType}*>({paramName})'

                            if bIsTypeStruct:
                                deepDestroyExpr+=f'if({memberName}) {memberName}->deepDestroy();\n\t\t'
                                memberPostInitializeInDeepCopyConstructor=f'if({memberName}) {memberName}->deepCopy(*{paramName});'
                                

                            deepDestroyExpr = f'ptr::safe_delete({memberName});'

                            memberConstructorExpr=f'{memberName}{{ {inParam} }}' 
                            memberDeepCopyConstructorExpr=f'{memberName}{{ ptr::safe_new_copy({inParam}) }}'

                            

                            # is handle, should replace  before execute
                            if param.type in parser.handles:
                                if sCommandType=='create':
                                # vkCreate vkAllocate
                                    commandBeforeExecuteExpr=f'{param.type} {memberName}New = VK_NULL_HANDLE;'
                                    commandExecuteParamExpr=f'&{memberName}New'
                                    commandAfterExecuteExpr=f'uint64_t {memberName}Store = (uint64_t){memberName}New;'
                                    commandAfterExecuteExpr+=f'\n\t\tif({memberName}) {handleProcess}(*{memberName}, {memberName}Store, EHandleOperateType::create);'
                                else:
                                    #raise ValueError("is handle pointer but not create or allocate")
                                    print(f"{name} {fulltype} is handle pointer but not create or allocate")
                                    pass

                        else:
                            # pAllocator
                            commandExecuteParamExpr=f'nullptr'
                            memberName=''
                            toStringExpr = f'RS_TO_STRING_WITH_NAME(pAllocator, kNullptrString);'

                    else:
                        # no-pointer normal param

                        if param.array!='':
                            # T a[N] 
                            memberDeclExpr=f'{memberType} {memberName}Arr{param.array}; /* T[N] */' 
                            memberDeepCopyConstructorExpr=memberConstructorExpr=f'{memberName}Arr{{}}' 
                            commandExecuteParamExpr=f'{memberName}Arr'
                            deserializeParam = serializeParams = f'{memberName}Arr'
                            memberPostInitializeInDeepCopyConstructor = memberPostInitializeInConstructor=f'memcpy({memberName}Arr, {paramName}, sizeof({paramType}{param.array}));'

                            toStringExpr = f'RS_TO_STRING({memberName}Arr);'
                        else: 
                            # T
                            memberDeclExpr=f'{memberType} {memberName}; /* T */' 
                            memberConstructorExpr=f'{memberName}{{{paramName} }}' 
                            commandExecuteParamExpr=f'{memberName}'

                            toStringExpr = f'RS_TO_STRING({memberName});'

                            if bIsTypeStruct:
                                deepDestroyExpr=f'{memberName}.deepDestroy();\n\t\t'
                                commandBeforeExecuteExpr=f'{memberName}.processHandle(handleReplace);'
                             
                            # is handle, should replace  before execute
                            if param.type in parser.handles:
                                #if sCommandType!='create':
                                    commandBeforeExecuteExpr=f'uint64_t {memberName}Transform = 0;'
                                    commandBeforeExecuteExpr+=f'\n\t\t{handleProcess}({memberName}, {memberName}Transform, EHandleOperateType::replace);'
                                    commandExecuteParamExpr=f'({param.type}){memberName}Transform'
                                    if sCommandType=='destroy':
                                        if name.find("vkFree")!=-1:
                                            if (name.endswith('s') and len(command.params)==4 and index>1) or (len(command.params)==3 and index>0):
                                                commandAfterExecuteExpr=f'{handleProcess}({memberName}, GUnusedHandle, EHandleOperateType::destroy);'
                                        elif firstParamTypeName == 'VkInstance' or firstParamTypeName == 'VkDevice' and (index>0 or len(command.params)==2):
                                            # vkDestroy* vkFree* object other than VkInstance and VkDevice
                                            commandAfterExecuteExpr=f'{handleProcess}({memberName}, GUnusedHandle, EHandleOperateType::destroy);'
                                        
                else:
                    # const char*, const char* [], const T*, const T*[] => std::string, std::vector<std::string>, std::vector<T>

                    memberName = GetRemovePointerName(paramName)
                    memberLength = f'{memberName}Length' 

                    if param.arrayCategory == EArrayType.String:
                        # array replace with std::vector / std::string
                        memberDeclExpr=f'char const* {memberName}; /* raw-string */'
                        memberConstructorExpr=f'{memberName}{{{paramName}}}' 
                        commandExecuteParamExpr = f'{memberName}'

                        deserializeParam = serializeParams = f'{memberName}, kReadWriteOptional'
                        deepDestroyExpr = f'ptr::safe_delete({memberName});'

                        toStringExpr = f'RS_TO_STRING({memberName});'

                        memberDeepCopyConstructorExpr=f'{memberName}{{ptr::safe_new_copy({paramName})}}'
                        
                    else:
                        length = param.lengthMember
                        lengthName = ''
                        lengthNameWithPrefix=param.lengthParameters[0]
                        lengthType=''
                        lengthExecutePrefix=''
                        if length == None:
                            lengthName = param.lengthParameters[0]
                        elif length.name:
                            lengthName = length.name
                            lengthType = length.type
                            # length is pointer
                            if length.pointer!='' and length.arrayCategory == EArrayType.Other:
                                lengthNameWithPrefix=f'{lengthName} ? *{lengthName}: 0'
                                lengthExecutePrefix='&'
                        else:
                            raise ValueError(f'function {name} param {param.name} length param is None')
                        
                        memberLengthNameRemovePointer = GetRemovePointerName(lengthName)
                        if param.arrayCategory == EArrayType.Array:
                            # raw-bytes + size
                            if memberType == 'void':
                                memberDeclExpr =f'uint8_t* {memberName}; /* raw-bytes + size */'
                                memberDeclExpr +=f'\n\tuint64_t {memberName}Length;'

                                memberConstructorExpr=f'{memberName}{{(uint8_t*){paramName}}}'
                                memberDeepCopyConstructorExpr=f'{memberName}{{ptr::safe_new_copy_array((uint8_t*){paramName}, {lengthNameWithPrefix})}}'
                                lengthExpr =f'\n\t\t, {memberName}Length{{{lengthNameWithPrefix}}}'
                                memberConstructorExpr+=lengthExpr
                                memberDeepCopyConstructorExpr+=lengthExpr
 

                                deserializeParam = serializeParams = f'{memberName}, {memberName}Length, kReadWriteLength_Allocate'
                                deepDestroyExpr = f'ptr::safe_delete_array({memberName}, {memberName}Length);'

                                toStringExpr = f'RS_TO_STRING({memberName});'
                                toStringExpr += f'\n\t\tRS_TO_STRING({memberName}Length);'
 
                                commandBeforeExecuteExpr=f'void* pData = {memberName};\n\t\t'
                                commandBeforeExecuteExpr+=AddLengthBeforeCommand(memberLengthNameRemovePointer, f'{lengthType} {memberLengthNameRemovePointer} = {memberName}Length;')
                                commandExecuteParamExpr=f'pData'
                                
                            # T* + length
                            else:
                                memberDeclExpr=f'{memberType}* {memberName}; /* T* + length */'
                                memberDeclExpr+=f'\n\tuint32 {memberName}Length;'
                                deserializeParam = serializeParams = f'{memberName}, {memberName}Length, kReadWriteLength_Allocate'
                                deepDestroyExpr += f'\n\t\tptr::safe_delete_array({memberName}, {memberName}Length);'

                                toStringExpr = f'RS_ARRAY_TO_STRING({memberName}, {memberName}Length);'
                                
                                inParam = paramName
                                if bIsStruct:
                                    inParam = f'castDerived<{memberType}>({paramName})'
                                elif param.prefix.find('const')!=-1 :
                                    memberConstructorExpr=f'{memberName}{{  }}'
                                    inParam = f'const_cast<{memberType}*>({paramName})'

                                memberConstructorExpr=f'{memberName}{{{inParam}}}'
                                memberDeepCopyConstructorExpr=f'{memberName}{{ptr::safe_new_copy_array({inParam}, {lengthNameWithPrefix})}}'

                                if bIsTypeStruct:
                                    memberPostInitializeInDeepCopyConstructor=f'for(uint32 i=0; i<{memberName}Length; ++i) {memberName}[i].deepCopy({paramName}[i]);'
                                    deepDestroyExpr=f'for(uint32 i=0; i<{memberName}Length; ++i) {memberName}[i].deepDestroy();'
                                    commandBeforeExecuteExpr=f'for(uint32 i=0; i<{memberName}Length; ++i) {memberName}[i].processHandle(handleReplace);'

                                lengthExpr = f'\n\t\t, {memberName}Length{{{lengthNameWithPrefix} }}'
                                memberConstructorExpr+=lengthExpr
                                memberDeepCopyConstructorExpr+=lengthExpr

                                commandBeforeExecuteExpr+=AddLengthBeforeCommand(memberLengthNameRemovePointer, f'{lengthType} {memberLengthNameRemovePointer} = {memberName}Length;')
                                
                                if param.type in parser.handles:
                                    # VkHandle array
                                    
                                    commandExecuteParamExpr=f'{memberName}Transforms.data()'
                                    if sCommandType=='create':
                                        commandBeforeExecuteExpr+=f'\n\t\tstd::vector<{param.type}> {memberName}Transforms{{{memberName}Length, VK_NULL_HANDLE}};'
                                        commandAfterExecuteExpr=f'\n\t\tfor(uint32 i=0; i<{memberName}Length; ++i){{\n\t\t\t/*ARAssert({memberName}Transforms[i]!=VK_NULL_HANDLE);*/'\
                                                f'\n\t\t\tuint64_t temp=(uint64_t){memberName}Transforms[i];\n\t\t\t{handleProcess}({memberName}[i], temp, EHandleOperateType::create);'\
                                                    f'\n\t\t}} '
                                    else:
                                        commandBeforeExecuteExpr+=f'\n\t\tstd::vector<{param.type}> {memberName}Transforms{{}};'\
                                            f'\n\t\tfor(uint32 i=0; i<{memberName}Length; ++i){{\n\t\t\tuint64_t temp = 0;'\
                                                f'\n\t\t\t{handleProcess}({memberName}[i], temp, EHandleOperateType::replace);'\
                                                    f'\n\t\t\t{memberName}Transforms.emplace_back(({param.type})temp);\n\t\t}} '
                                        if sCommandType=='destroy':
                                            commandAfterExecuteExpr=f'\n\t\tfor(uint32 i=0; i<{memberName}Length; ++i){{'\
                                                f'\n\t\t\t{handleProcess}({memberName}[i], GUnusedHandle, EHandleOperateType::destroy);'\
                                                    f'\n\t\t}} '
                                else:
                                    # normal data array
                                    commandExecuteParamExpr=f'{memberName}'

                        elif param.arrayCategory == EArrayType.StringArray:
                            memberDeclExpr=f'char const* const* {memberName}; /* raw-string array */'
                            memberDeclExpr+=f'uint32 {memberName}Length;'
                            memberConstructorExpr=f'{memberName}{{{paramName} }}'
                            memberDeepCopyConstructorExpr=f'{memberName}{{ptr::safe_new_copy_array({paramName}, {lengthName})}}'

                            lengthExpr+=f'\n\t\t, {memberName}Length{{{lengthName} }}'
                            memberConstructorExpr+=lengthExpr
                            memberDeepCopyConstructorExpr+=lengthExpr

                            deserializeParam = serializeParams = f'{memberName}, {memberName}Length, kReadWriteLength_Allocate'
                            deepDestroyExpr = f'ptr::safe_delete_array({memberName}, {memberName}Length);'

                            commandBeforeExecuteExpr=AddLengthBeforeCommand(memberLength, f'{lengthType} {memberLength} = {memberName}Length;\n\t\t')
                            commandExecuteParamExpr=f'{memberName}'

                            toStringExpr = f'RS_TO_STRING({memberName});'

                        else:
                            memberDeclExpr=f'/* {param.type} {paramName} type not found */'
                            raise ValueError('type not found')

                if bIsAllocator:
                    commandExecuteParamExpr='nullptr'
                    serializeParams=deserializeParam=''
                    memberDeepCopyConstructorExpr=''

                constructorParamExprList+=f'{dot} {fulltype} {paramName}{param.array}'
                if commandExecuteParamExpr!='': commandExecuteParamExprList+=f'{dot}{commandExecuteParamExpr}'
                if commandBeforeExecuteExpr!='': commandBeforeExecuteExprList+=f'\n\t\t{commandBeforeExecuteExpr}'
                if commandAfterExecuteExpr!='': commandAfterExecuteExprList+=f'\n\t\t{commandAfterExecuteExpr}'
                if memberPostInitializeInConstructor!='': memberPostInitializeInConstructorList+=f'\n\t\t{memberPostInitializeInConstructor}'
            
                if param.bIsLength:
                    # member length don't generate decl/constructor/serialize/deserialize but need constructor param/commandExecute
                    if param.name:
                        lengthName = param.name
                        # length is pointer
                        if param.pointer!='': commandExecuteParamExpr=f'&{lengthName}'
                        else: commandExecuteParamExpr=f'{lengthName}'
                    else:
                        raise ValueError('{param.name} is length but name is none')
                else:
                    if memberDeclExpr!='': memberDeclExprList+=f'\n\t{memberDeclExpr}'
                    if memberConstructorExpr!='': memberConstructorExprList+=f'\n\t\t, {memberConstructorExpr}'
                    if toStringExpr!='': toStringExprList+=f'\n\t\t{toStringExpr}'
                    if exprBeforeSerialize!='':
                        serializeExprList+=exprBeforeSerialize
                    if exprBeforeDeserialize!='':
                        deserializeExprList+=exprBeforeDeserialize
                    if serializeParams!='':
                        serializeExprList+=f'\n\t\tCHECK_SERIALIZE_SUCC(file->write({serializeParams}));'
                    if deserializeParam!='':
                        deserializeExprList+=f'\n\t\tCHECK_SERIALIZE_SUCC(file->read({deserializeParam}));'
                    if deepDestroyExpr!='':
                        deepDestroyExprList+=f'\n\t\t{deepDestroyExpr}'

                    if memberDeepCopyConstructorExpr!='':
                        memberDeepCopyConstructorExprList+=f'\n\t\t, {memberDeepCopyConstructorExpr}'

                    if memberPostInitializeInDeepCopyConstructor!='':
                        memberPostInitializeInDeepCopyConstructorList+=f'\n\t\t{memberPostInitializeInDeepCopyConstructor}'
                    
            if commandBeforeExecuteExprList!='':
                commandBeforeExecuteExprList+='\n\t\t'
            if commandAfterExecuteExprList!='':
                commandAfterExecuteExprList+='\n\t\t'
            if deepDestroyExprList!='':
                deepDestroyExprList=f'{deepDestroyExprList}\n\t'
            if memberDeepCopyConstructorExprList!='':
                memberDeepCopyConstructorExprList=f'{memberDeepCopyConstructorExprList}\n\t'
            if memberPostInitializeInDeepCopyConstructorList!='':
                memberPostInitializeInDeepCopyConstructorList+='\n\t'
            if toStringExprList!='':
                toStringExprList+='\n\t'

            
            dispatchTable=''
            if command.commandType == ECommandType.Global:
                dispatchTable = 'GGlobalCommands()'
            elif command.commandType == ECommandType.Instance:
                dispatchTable = 'GInstanceCommands()'
            elif command.commandType == ECommandType.Device:
                dispatchTable = 'GDeviceCommands()'
            else:
                raise ValueError("unkown command type")

            if name=='vkQueueSubmit':
                commandAfterExecuteExprList+=f'\n\t\tVkFence waitFence=(VkFence)fenceTransform;\n\t\t'\
                    f'ret = {dispatchTable}->vkWaitForFences({dispatchTable}->handle, 1, &waitFence, true, 1000000000);\n\t\tEnsureVulkanResult(ret);'


            if name == 'vkFlushMappedMemoryRanges':
                memberDeclExprList+=f'\n\tuint8* pData;'
                memberDeclExprList+=f'\n\tuint64 dataLength;'
                memberDeepCopyConstructorExprList+=f'\t, pData{{nullptr}}\n\t'
                memberDeepCopyConstructorExprList+=f'\t, dataLength{{0}}\n\t'
                memberPostInitializeInDeepCopyConstructorList+='''
        // total length
		for(uint32 i=0; i<memoryRangesLength; ++i){
			dataLength+=pMemoryRanges[i].size;
		}
		pData = ptr::safe_new_default_array<uint8>(dataLength);
                '''
                deepDestroyExprList+='ptr::safe_delete_array(pData, dataLength);'
                serializeExprList+='\n\t\tCHECK_SERIALIZE_SUCC(file->write(pData, dataLength, kReadWriteLength_Allocate));'
                deserializeExprList+='\n\t\tCHECK_SERIALIZE_SUCC(file->read(pData, dataLength, kReadWriteLength_Allocate));'

            # all components
            beginSerializeExpr='\n\t\tRS_BEGIN_BASE_SERIALIZE'
            beginDeserializeExpr='\n\t\tRS_BEGIN_BASE_DESERIALIZE'
            endSerializeExpr ='\n\t\tRS_END_SERIALIZE'
            defaultConstructor=f'{structName}()=default;'
            Destructor=f'~{structName}() override{{conditionalDestroy();}};'
            constructor = f'{structName}({constructorParamExprList})\n\t\t{parentInitalize}{memberConstructorExprList}\n\t{{{memberPostInitializeInConstructorList}\n\t}}'
            deepCopyConstructor = f'{structName}({constructorParamExprList}, DeepCopy)\n\t\t{parentDeepCopyInitalize}{memberDeepCopyConstructorExprList}{{{memberPostInitializeInDeepCopyConstructorList}\n\t}}'

            retExpr=''
            retCheckExpr=''
            if command.ret not in ('PFN_vkVoidFunction','void','VkDeviceAddress','uint64_t'):
                retExpr='VkResult ret ='
                retCheckExpr='\n\t\tCheckVulkanResult(ret);'

            toStringFunction=''f'virtual std::string toString() const override{{\n\t\tRS_BEGIN_DERIVED_TO_STRING({structName},{parentName});{toStringExprList};\n\t\tRS_END_TO_STRING();\n\t}}'
            serializeFunction=f'virtual bool serialize(File* file) const override{{{beginSerializeExpr}{serializeExprList}{endSerializeExpr}\n\t}}'
            deserializeFunction=f'virtual bool deserialize(File* file) override{{{beginDeserializeExpr}{deserializeExprList}{endSerializeExpr}\n\t}}'

            executeBeginCheck=''


            if parser.isExtension:
                executeBeginCheck=f'\n\t\tCheckAPIExists({dispatchTable}->{name});'
            else:
                executeBeginCheck=f'\n\t\tARAssert({dispatchTable}->{name}!=nullptr);'
                

            executeCommandFunction=f'virtual void execute(HandleReplace&& handleReplace) override {{{executeBeginCheck}{commandBeforeExecuteExprList}{retExpr}'\
                f'{dispatchTable}->{name}({commandExecuteParamExprList});{retCheckExpr}{commandAfterExecuteExprList}\n\t}}'
            #executeCommandFunction=f'virtual void execute(HandleReplace&& handleReplace) override {{{commandBeforeExecuteExprList}callDispatch("{name}", *this, {commandExecuteParamExprList});{commandAfterExecuteExprList}\n\t}}'
            
            declareDeriveMarco=f'\n\tRS_DECLARE_OBJECT_INFO_CLASS({structName}, {parentName})'
            deepDestroyFunction=f'virtual void deepDestroy() override{{{deepDestroyExprList}}}'

            

            # struct
            structDefine = f'\n\n/* {name}({command.getDeclare()[0]}) */'
            structDefine += f'\nstruct {structName}{parentDerive}{{{commandTypeExpr}{declareDeriveMarco}\n\t{memberDeclExprList}\n\t'\
                f'{defaultConstructor}\n\t{constructor}\n\t{deepCopyConstructor}\n\t{Destructor}\n\t{toStringFunction}\n\t{serializeFunction}'\
                    f'\n\t{deserializeFunction}\n\t{executeCommandFunction}\n\t{deepDestroyFunction}\n}};'

            # add to final
            SectionContentsHeader+=structDefine         

            notEnumerateCommand = (
                "vkCreateDevice",
                "vkCreateInstance",
                "vkMapMemory",
                "vkUnmapMemory",
                "vkAllocateMemory",
                "vkFreeMemory",
                "vkFlushMappedMemoryRanges",
                "vkQueueSubmit",
                "vkFreeDescriptorSets",
                "vkUpdateDescriptorSets",
                "vkCmdBindDescriptorSets",
                "vkFreeCommandBuffers",
                "vkEndCommandBuffer",
                "vkBeginCommandBuffer",
                "vkBindImageMemory",
                "vkBindBufferMemory",
                "vkCreateImage",
                "vkDestroyImage",
                "vkCreateBuffer",
                "vkDestroyBuffer",
                'vkCmdBindVertexBuffers',
                'vkCmdBindIndexBuffer',
                'vkCreateImageView',
                'vkDestroyImageView',
                'vkCreateBufferView',
                'vkDestroyBufferView',
                'vkCmdCopyBufferToImage',
                'vkCmdDraw',
                'vkCmdDrawIndexed',
                'vkCmdDrawIndirect',
                'vkCmdDispatch',
                'vkCmdDispatchIndirect',
                'vkCmdDrawIndexedIndirect',
                'vkCreateGraphicsPipelines',
                'vkDestroyPipeline',
                'vkCreateComputePipelines',
                'vkCmdBindPipeline',
            )

            if name not in notEnumerateCommand:
                SectionCommandEnumlator+=f'\\\n\t\t\tMacro({name}, {structName}, {command.getDeclare()[2]})'

        def GenerateStruct(parser:VkXmlParser, name:str, struct:VkType):
            global SectionContentsHeader,SectionContentsCPP, MappingNames

            if struct.category != ETypeCategory.Struct:
                return

            if name in MappingNames:
                return 

            if name == 'VkBaseInStructure' or name == 'VkBaseOutStructure':
                return 
            

            # only vkstruct has pNext 
            bHasVkStructType = False
            if struct.sType!='':
                bHasVkStructType=True 

            structName = GetNewStructName(name, struct.bIsPod)

            nextStructComment=''
            nextSerializeStructComment=''
            if struct.nextStructs!=None:
                for nextName, nextstruct in struct.nextStructs.items():
                    nextStructComment += f' {nextName}'
                    nextSerializeStructComment+=f' {GetDependsStructName(nextName)}'

            parentType =name
            infoName='info'
            infoDot=f'{infoName}.'
            handleProcess = 'VkHandleProcess'
         
            parentDeriveExpr=f':public {parentType}'
            parentConstructorExpr=f': {parentType}{{{infoName}}}'

            memberDeclExprList=''
            memberConstructorExprList=''
            DeepCopyConstructorContentExprList=''
            destructorParamExprList=''
            
            toStringExprList=''
            serializeExprList=''
            deserializeExprList=''
            replayExprList=''

            setupReferenceExprList=''
            if struct.sType!='':
                setupReferenceExprList='\n\t\tsType = kStructType;'

            arrayReadMark='kReadWriteLength_Allocate, false'
            arrayWriteMark='kReadWriteLength_Allocate'

            processedLengthParams={}
            
            for param,index in zip(struct.members.values(), range(len(struct.members))): 
                # member name, remve all p and lower then first char
                paramName=param.name
                paramTypeName=param.type 

                if paramTypeName =='VkPipelineShaderStageCreateInfo':
                    print("")
                paramType:VkType=None
                bCheckLengthRead = False
                if paramTypeName!='':
                    paramType=parser.getTypeByName( paramTypeName)

                fullTypeName = param.getFullType()

                Dot=''
                if index!=0:
                    Dot=','

                # remove allocator
                if paramName == 'pAllocator':
                    continue

                if param.name == 'sType':
                    continue
 

                # is pointer type
                bIsPointer = False 
                if param.pointer!='':
                    bIsPointer = True

    
                memberDeclExpr=''
                memberConstructorExpr=''
                DeepCopyConstructorContentExpr=''
                setupReferenceExpr=''
                localExecuteContentExpr=''
                destructorExpr=''
                replayExpr=''
                
                # another struct type
                
                bStructType=False
                memberTypeName = paramTypeName

                memberType=None
                if param.type!=None and param.type in parser.types:
                    memberType = parser.types[param.type ]
                    if memberType.category == ETypeCategory.Struct:
                        #if bIsPointer:
                        #if not tryMemberType.bIsPod:
                        memberTypeName = GetDependsStructName(memberType.name)
                    bStructType=True

                def isStructType(typeName):
                    if typeName!=None and typeName in parser.types and parser.types[ typeName ]:
                        return True 
                    return False
                    
            
                serializeName=param.name
                deserializeName=param.name
                toStringExpr=f'RS_TO_STRING({param.name})'
                serializeExpr=''
                deserializeExpr=''
                serializeCreate=''
                deserializeCreate=''
                
                inParamName = f'{infoDot}{paramName}'

                lengthName = ''
                if len(param.lengthParameters)>0:
                    lengthName = f'{param.lengthParameters[0]}'
                inLengthName = f'{infoDot}{lengthName}'
        
                
                if param.lengthMember==None or param.lengthMember.altlen!='':
                    inLengthName = lengthName

                if param.name=='pNext':
                    # pNext 
                    DeepCopyConstructorContentExpr=f'pNext = VkLocalStruct::deepCopy({inParamName});'
                    destructorExpr=f'VkLocalStruct::deepDestroy(pNext); /* pNext */'
                    setupReferenceExpr=''
                    serializeExpr = 'VkLocalStruct::serializeNext(file, pNext)'
                    deserializeExpr = 'VkLocalStruct::deserializeNext(file, &pNext)'
                    toStringExpr = f'RS_TO_STRING(pNext);'
                elif param.arrayCategory == EArrayType.String:
                    # raw-string
                    DeepCopyConstructorContentExpr = f"{paramName} = RawCharString::copy({inParamName});"
                    destructorExpr=f'ptr::safe_delete({paramName}); /* raw-string */'

                    deserializeName = serializeName=f"{paramName}, kReadWriteOptional"
                    toStringExpr = f'RS_TO_STRING({paramName});'
                elif param.arrayCategory == EArrayType.StringArray:
                    # raw-string array

                    # deep copy
                    DeepCopyConstructorContentExpr = f"{paramName} = ptr::safe_new_copy_array({inParamName},{inLengthName});"
                    destructorExpr=f'ptr::safe_delete_array({paramName}, {lengthName}); /* raw-string array */'

                    # serialize
                    serializeName = f"{paramName}, {lengthName}, {arrayWriteMark}" 
                    deserializeName =f"{paramName}, {lengthName}, {arrayReadMark}" 
                    toStringExpr = f'RS_ARRAY_TO_STRING({paramName}, {lengthName});'
                   
                    bCheckLengthRead= param.altlen!=''
                elif paramType!=None and not param.bIsLength and bIsPointer:
                    # manully allocate and delete 
                    if param.arrayCategory == EArrayType.Array:
                        if paramTypeName=='void':
                            # raw bytes
                            DeepCopyConstructorContentExpr = f"{paramName} = ptr::safe_new_copy_array({inParamName}, {inLengthName});"
                            destructorExpr=f'ptr::safe_delete_array({paramName}, {lengthName}); /* raw bytes */'

                            serializeName = f"{paramName}, {lengthName}, {arrayWriteMark}" 
                            deserializeName =f"{paramName}, {lengthName}, {arrayReadMark}" 
                            toStringExpr = f'RS_TO_STRING({paramName});'
                        else:
                            # const T* + length 
                            length = lengthName
                            if param.altlen!='' and length not in processedLengthParams:
                                processedLengthParams[length]=1
                                length = f'{param.name}Length'
                                deserializeCreate=f'uint32_t {length} = {lengthName};\n\t\t'

                            
                            serializeName=f"typedPointer<{memberTypeName}>({paramName}), {lengthName}, {arrayWriteMark}"
                            deserializeName=f"*typedPointer<{memberTypeName}*>(&{paramName}), {length}, {arrayReadMark}"
                            DeepCopyConstructorContentExpr=f'{param.name} = ptr::safe_new_copy_array( {inParamName}, {inLengthName});'
                            if paramType.category == ETypeCategory.Struct:
                                DeepCopyConstructorContentExpr+=f'\n\t\tif({inParamName}!=nullptr && {inLengthName}>0){{\n\t\t\tARAssert({param.name}!=nullptr);\n\t\t\tfor(uint32 i=0; i<{inLengthName}; ++i){{typedPointer<{memberTypeName}>({paramName})[i].deepCopy({inParamName}[i]);}}\n\t\t}}' 
                                replayExpr=f'if({paramName}!=nullptr &&{length}>0){{\n\t\t\tfor(uint32 i=0; i<{length}; ++i){{typedPointer<{memberTypeName}>({paramName})[i].processHandle(handleReplace);}}\n\t\t}}'
                            destructorExpr=f'ptr::safe_delete_array({param.name}, {lengthName}); /* array<T*> */'

                            toStringExpr = f'RS_ARRAY_TO_STRING_WITH_NAME({paramName}, typedPointer<{memberTypeName}>({paramName}), {lengthName});'

                            if memberType!=None and memberTypeName in parser.handles:
                                # handle
                                replayExpr=f'if({paramName}!=nullptr){{'
                                replayExpr+=f'\n\t\t\tfor(uint32 i=0; i<{length}; ++i){{\n\t\t\tuint64_t temp = 0 ;'
                                replayExpr+=f'\n\t\t\t\t{handleProcess}({paramName}[i], temp, EHandleOperateType::replace);'
                                replayExpr+=f'\n\t\t\t\tconst_cast<{memberTypeName}*>({paramName})[i] = ({memberTypeName})temp;\n\t\t}}}}'

                            bCheckLengthRead=True

                    elif paramType.category == ETypeCategory.Struct :
                        #filter out base type like int32_t
                        warpType = f'typedPointer<{memberTypeName}>({paramName})'
                        wrapRefType = f'*typedPointer<{memberTypeName}*>(&{paramName})'
                        DeepCopyConstructorContentExpr =f'{param.name} = ptr::safe_new_copy({inParamName});'\
                            f'\n\t\tif({inParamName}!=nullptr){{ ARAssert({param.name}!=nullptr); {warpType}->deepCopy(*{inParamName});}}' 
                        destructorExpr=f'ptr::safe_delete({wrapRefType});'
                        serializeName=f'{warpType}, kReadWriteOptional' 
                        deserializeName=f'{wrapRefType}, kReadWriteOptional'
                        toStringExpr = f'RS_TO_STRING_WITH_NAME({paramName}, {warpType});'
                        replayExpr=f'if({paramName}!=nullptr) {warpType}->processHandle(handleReplace);'
                    else:
                        destructorExpr=f'/*uncatch pointer {fullTypeName} {param.name} {param.array}*/'
                        print(f"#### serialize::GenerateDependsStruct uncatch pointer {fullTypeName} {param.name} {param.array}")
                else:
                    
                    if bIsPointer:
                        DeepCopyConstructorContentExpr =f'{param.name} = {inParamName} ? ptr::safe_new_copy({inParamName}): 0;' 
                        destructorExpr=f'ptr::safe_delete({param.name}); /* raw-pointer */'
                        deserializeName = serializeName={param.name}
                        toStringExpr = f'RS_TO_STRING({param.name});'

                        if memberType!=None and memberTypeName in parser.handles:
                            # handle
                            replayExpr=f'uint64_t {paramName}New=0;'
                            replayExpr+=f'\n\t\t{handleProcess}({paramName}, {paramName}New, EHandleOperateType::replace);'
                            replayExpr+=f'\n\t\t{paramName} = ({memberTypeName}){paramName}New;'

                    else:
                        destructorExpr=f'/*other type: {fullTypeName} {param.name} {param.array}*/'
                        print(f'other type: {fullTypeName} {param.name} {param.array}')
      
                        #if bStructType:
                        if paramType.category == ETypeCategory.Struct and param.array=='':
                            warpType = f'typedPointer<{memberTypeName}>(&{param.name})'
                            toStringExpr = f'RS_TO_STRING_WITH_NAME( {param.name}, *{warpType} );'
                            serializeName=f'*{warpType}' 
                            deserializeName=f'*{warpType}'
                            DeepCopyConstructorContentExpr =f'{warpType}->deepCopy({inParamName});'
                            destructorExpr=f'{warpType}->deepDestroy(); /* deep T */'

                            replayExpr=f'{warpType}->processHandle(handleReplace);'

                        if memberType!=None and memberTypeName in parser.handles:

                            if param.array!='':
                                # handle
                                replayExpr=f'uint64_t {paramName}New=0; /* array */' 
                            else:
                                # handle
                                replayExpr=f'uint64_t {paramName}New=0;'
                                replayExpr+=f'\n\t\t{handleProcess}({paramName}, {paramName}New, EHandleOperateType::replace);'
                                replayExpr+=f'\n\t\t{paramName} = ({memberTypeName}){paramName}New;'    
                            
                            

                    # param.name == 'pSetLayouts'

                if param.bIsLength:
                    processedLengthParams[paramName]=1
                if bCheckLengthRead and param.lengthParameters[0] not in processedLengthParams:
                    raise ValueError(f"{serializeName} read but length{param.lengthParameters[0]} not found ")

                if setupReferenceExpr!='':
                    setupReferenceExprList+=f'\n\t\t{setupReferenceExpr} /* {fullTypeName} */\n\t'
                
                if destructorExpr!='':
                    destructorParamExprList+=f'\n\t\t{destructorExpr}'

                if memberDeclExpr!='':
                    memberDeclExprList+=f'\n\t{memberDeclExpr};'

                if memberConstructorExpr!='':
                    if parentConstructorExpr!='':
                        memberConstructorExpr =f'\n\t\t{Dot} {memberConstructorExpr}'
                    else:
                        memberConstructorExpr =f' {memberConstructorExpr}'
                memberConstructorExprList+=memberConstructorExpr

                if DeepCopyConstructorContentExpr!='':
                    DeepCopyConstructorContentExprList+=f'\n\t\t{DeepCopyConstructorContentExpr}'

                toStringExprList+=f''
                
                if serializeCreate!='':
                    serializeExprList+=f'\n\t\t{serializeCreate}'
                
                if serializeExpr!='':
                    serializeExprList+=f'\n\t\tCHECK_SERIALIZE_SUCC({serializeExpr});'
                elif serializeName!='':
                    serializeExprList+=f'\n\t\tCHECK_SERIALIZE_SUCC(file->write({serializeName}));'

                if deserializeCreate!='':
                    deserializeExprList+=f'\n\t\t{deserializeCreate}'
                
                if deserializeExpr!='':
                    deserializeExprList+=f'\n\t\tCHECK_SERIALIZE_SUCC({deserializeExpr});'
                elif deserializeName!='':
                    deserializeExprList+=f'\n\t\tCHECK_SERIALIZE_SUCC(file->read({deserializeName}));'
                
                if toStringExpr!='':
                    toStringExprList+=f'\n\t\t{toStringExpr}'
                if replayExpr!='':
                    replayExprList+=f'\n\t\t{replayExpr}'
 
            implStructScope=f'{structName}::'

            macroDefine=''
            setupReferenceFunction=''
            callSetup =''
            if bHasVkStructType:
                macroDefine=f'\n\tRS_DECLARE_VK_TYPE_STRUCT({structName},{parentType}, {struct.sType});'
                setupReferenceFunction=f'void setup(){{{setupReferenceExprList}\n\t}}'
                callSetup='setup();'
            else:
                macroDefine=f'\n\tRS_DECLARE_VK_POD_STRUCT({structName});'
                
            
            beginNormalSerializeExpr='\n\t\tRS_BEGIN_SERIALIZE' 
            endSerializeExpr ='\n\t\tRS_END_SERIALIZE\n'
            defaultConstructor=f'{structName}():{parentType}{{}}{{{callSetup}}};'
 
            copyConstructorDecl = f'{structName}({name} const& info){parentConstructorExpr}{{}}'
            destructorDecl = f'~{structName}()=default;'
            toStringDecl = f'std::string toString() const;'
            toStringFunction=f'std::string {implStructScope}toString() const{{\n\t\tRS_BEGIN_TO_STRING({structName});{toStringExprList}\n\t\tRS_END_TO_STRING();\n\t}}'
            
 
            serializeFunctionDecl=''
            deserializeFunctionDecl=''
            serializeFunctionImpl=''
            deserializeFunctionImpl=''
 
            serializeFunctionDecl=f'bool serialize(File* file) const;'
            deserializeFunctionDecl=f'bool deserialize(File* file);'
            serializeFunctionImpl=f'bool {implStructScope}serialize(File* file) const {{{beginNormalSerializeExpr}{serializeExprList}{endSerializeExpr}}}'
            deserializeFunctionImpl=f'bool {implStructScope}deserialize(File* file) {{{beginNormalSerializeExpr}{deserializeExprList}\n\t\t{callSetup}{endSerializeExpr}}}'
            staticCheck=f'static_assert(sizeof({structName}) == sizeof({name}) && std::is_standard_layout_v<{structName}>, "size should equal");'

            if struct.name == 'VkDebugMarkerObjectNameInfoEXT':
                replayExprList+=f'\n\t\tuint64_t newObject=0; VkHandleProcess(object, newObject, EHandleOperateType::replace); object=newObject;'

            deepCopyExprDecl=f'void deepCopy({name} const& info);'
            deepCopyExprImpl=f'void {implStructScope}deepCopy({name} const& info){{\n\t\t/*memcpy(this, &info, sizeof({name}));*/{DeepCopyConstructorContentExprList}\n}}'
            deepDestroyDecl=f'void deepDestroy();'
            deepDestroyImpl=f'void {implStructScope}deepDestroy(){{{destructorParamExprList}\n}}'
            processHandleDecl=f'void processHandle(HandleReplace&& handleReplace);'
            processHandleImpl=f'void {implStructScope}processHandle(HandleReplace&& handleReplace){{{replayExprList}}}'


            if nextStructComment!='':
                nextStructComment=f'\n/* {nextStructComment} */'
            if nextSerializeStructComment!='':
                nextSerializeStructComment=f'\n/* {nextSerializeStructComment} */'

            structDefine = f'\n{nextStructComment}{nextSerializeStructComment}\nstruct {structName}{parentDeriveExpr}{{{macroDefine}{memberDeclExprList}\n\t'\
                f'{defaultConstructor}\n\t{copyConstructorDecl}\n\t{destructorDecl}\n\t{deepCopyExprDecl}\n\t{deepDestroyDecl}\n\t{toStringDecl}\n\t{serializeFunctionDecl}'\
                    f'\n\t{deserializeFunctionDecl}\n\t{processHandleDecl}\n\t{setupReferenceFunction}\n}};\n{staticCheck}'

            structImpl = f'\n{deepCopyExprImpl}\n{deepDestroyImpl}\n{serializeFunctionImpl}\n{deserializeFunctionImpl}\n{toStringFunction}\n{processHandleImpl}'
 
            SectionContentsHeader+=structDefine
            SectionContentsCPP+=structImpl
        
        def processEachFunctionality(parser:VkXmlParser, name:str, func:VkFunctionity):
            global SectionContentsHeader,AllContentsHeader, SectionContentsCPP,AllContentsCPP, SectionCommandEnumlator
            global AllFuncDefine, AllCommandEnumlator,StructsDepends
            protect = func.protect

            if SectionContentsHeader!='':
                SectionContentsHeader = WrapProtect(protect, SectionContentsHeader)
                SectionContentsCPP = WrapProtect(protect, SectionContentsCPP)

                AllContentsHeader += f"//{name}{SectionContentsHeader}\n\n"
                AllContentsCPP += f"//{name}{SectionContentsCPP}\n\n"

                SectionContentsCPP=SectionContentsHeader=''
            if SectionCommandEnumlator!='':
                upperName = name.replace(' ','_').upper()
                macroName = f'ENUMERATE_COMMAND_{upperName}(Macro)'
                defineMacro = WrapIfElseProtect(protect,macroName, SectionCommandEnumlator)
                
                # content accumulate
                AllFuncDefine +=f'\\\n\t\t{macroName}'
                AllCommandEnumlator+= f"//  {name}\n{defineMacro}\n\n"
                SectionCommandEnumlator=''

        # command
        parser.foreachFunctionity(processEachFunctionality,  typeCallback=GenerateStruct)
        parser.foreachFunctionity(processEachFunctionality, commandCallback=GenerateCommandStruct )
        
 
        CommandInfoHeader.write(
f'''
#include"vk_common.h"
// forward declare
PROJECT_NAMESPACE_BEGIN
{AllContentsHeader}
PROJECT_NAMESPACE_END

// command enumulater
{AllCommandEnumlator}

// functionality enumulator
#define ENUMERATE_VK_ALL_COMMAND(Macro){AllFuncDefine}

''')

        CommandInfoCPP.write(
f'''
#include"vk_serialize_defs.h"
// forward declare
PROJECT_NAMESPACE_BEGIN
{AllContentsCPP}
PROJECT_NAMESPACE_END

''')



"""
write vk_wrap_defs.h
"""
with open(scriptPath + 'vk_wrap_defs.h', mode='w', newline = None) as CommandInfo:
    parser.writeDeclare(CommandInfo)

    HandleObjectDefines = ''
    StructDefines =''
    SectionStructDefines=''
    DependsStructs =''

    # which struct depends
    StructsDepends:dict[str,str]={}

    # name 2 code
    Depends2Code:dict[str,str]={}
    MappingNames:dict[str,str]={}

    def GetStructName(name:str):
        newStructName = name
        #for prefix in ('Vk', 'vk'):
        if name.startswith('Vk'):
            newStructName = name.removeprefix('Vk')
            newStructName = f'W{newStructName}'

        elif name.startswith('vk'):
            newStructName = name.removeprefix('vk')
            newStructName = newStructName[0:1].lower() + newStructName[1:]
        return newStructName

    def GetNewStructName(name:str, protect:str=None):
        global MappingNames
        newStructName = GetStructName(name)
        MappingNames[name]=newStructName
        return newStructName
    

    def GetDependsStructName(name:str, protect:str=None):

        global MappingNames,StructsDepends
        final =''
        if name in MappingNames:
            final = MappingNames[name]
        else:
            structName= GetStructName(name)
            StructsDepends[name] = structName
            final = structName
        if final == 'WViewport':
            final ='VkViewport'
        return final 

    Struct2Depends={}
    Depends2Struct={}
    StructCode={}

    def GenerateDependsStruct(parser:VkXmlParser, name:str, struct:VkType):
        global StructDefines,MappingNames,StructCode,Struct2Depends,SectionStructDefines

        #if name in MappingNames:
        #    return 
        if struct.alias!='':
            if struct.alias in parser.structTypes:
                struct = parser.structTypes[struct.alias]

        if struct.category != ETypeCategory.Struct:
            return
            
        structName = GetNewStructName(name, struct.bIsPod)

        parentType =name
        parentDeriveExpr=f':public {parentType}'
        parentDefaultConstructorExpr=f': {parentType}{{}}'
 
        SetContentExprList=''

        processedLengthParams={}
        
        depends=[]
        for param,index in zip(struct.members.values(), range(len(struct.members))): 
            # member name, remve all p and lower then first char
            paramName=param.name
            paramTypeName=param.type 
            paramType:VkType=None
            bCheckLengthRead = False
            if paramTypeName!='':
                paramType=parser.getTypeByName( paramTypeName)

            fullTypeName = param.getFullType()

            Dot=''
            if index!=0:
                Dot=','

            # use default allocator
            if paramName == 'pAllocator':
                continue
            
            # use constexpr type
            if param.name == 'sType':
                continue
            
            # is pointer type
            bIsPointer = False 
            if param.pointer!='':
                bIsPointer = True

            # remove prefix and change to value
            memberName = paramName
            if bIsPointer:
                memberName=memberName.removeprefix('p')
                memberName=memberName.removeprefix('p')
                if memberName[0:1].isupper():
                    memberName = memberName[:1].lower() + memberName[1:]

            # generate depends
            typeName = param.type
            if param.type!=None and param.type in parser.types:    
                tryMemberType = parser.types[param.type ]
                if tryMemberType.category == ETypeCategory.Struct:
                    typeName=GetDependsStructName(tryMemberType.name)
                    depends.append(tryMemberType)


            constructorParamExpr=''
            ConstructorContentExpr=''
            lengthName = ''
            if len(param.lengthParameters)>0:
                lengthName = f'{param.lengthParameters[0]}'

            # array replace with std::vector / std::string
            if param.arrayCategory == EArrayType.String:
                constructorParamExpr=f'std::string {param.prefix}& {memberName}'
                ConstructorContentExpr=f'this->{param.name} = {memberName}.c_str();'
            elif param.arrayCategory == EArrayType.StringArray:
                constructorParamExpr=f'std::vector<char const*> {param.prefix}& {memberName}'

                if lengthName not in processedLengthParams:
                    ConstructorContentExpr=f'this->{lengthName} = {memberName}.size();'
                    processedLengthParams[lengthName]=1
                else:
                    ConstructorContentExpr=f'ARAssert( this->{lengthName} == {memberName}.size());'

                ConstructorContentExpr+=f'\n\t\tthis->{param.name} = {memberName}.data();'
                bCheckLengthRead= param.altlen!=''
            elif param.lengthMember!=None:
                # manully allocate and delete 
                if param.arrayCategory == EArrayType.Array:
                    if paramTypeName=='void':
                        # const void* + length
                        constructorParamExpr=f'std::vector<uint8_t> {param.prefix}& {memberName}'
                        if lengthName not in processedLengthParams:
                            ConstructorContentExpr=f'this->{lengthName} = {memberName}.size();'
                            processedLengthParams[lengthName]=1
                        else:
                            ConstructorContentExpr=f'ARAssert( this->{lengthName} == {memberName}.size());'
                        ConstructorContentExpr+=f'\n\t\tthis->{param.name} = (void*){memberName}.data();'
                    else :
                        # T* + length
                        constructorParamExpr=f'std::vector<{typeName}> {param.prefix}& {memberName}'
                        if lengthName not in processedLengthParams:
                            ConstructorContentExpr=f'this->{lengthName} = {memberName}.size();'
                            processedLengthParams[lengthName]=1
                        else:
                            ConstructorContentExpr=f'ARAssert( this->{lengthName} == {memberName}.size());'
                        ConstructorContentExpr+=f'\n\t\tthis->{param.name} = {memberName}.data();' 
                        

                elif paramType.category == ETypeCategory.Struct :
                    pass
                else:
                    print(f"uncatch pointer {fullTypeName} {param.name} {param.array}")
            else:
                if bIsPointer:
                    pass
                else:
                    #print(f'other type: {fullTypeName} {param.name} {param.array}')
                    pass

            if ConstructorContentExpr!='':
                if constructorParamExpr=='':
                    raise ValueError('can not be null')
                setFunctionName = memberName[0:1].upper() + memberName[1:]
                SetContentExprList+=f'\n\tvoid set{setFunctionName}({constructorParamExpr}){{\n\t\t{ConstructorContentExpr}\n\t}}'
 
        # sType 
        structSetupType=''
        if struct.sType!='': 
            structSetupType=f'\n\t\tsType = {struct.sType};\n\t'
         
        defaultConstructor=f'{structName}(){parentDefaultConstructorExpr}{{{structSetupType}}}'
   
        structDefine = f'\n\nstruct {structName}{parentDeriveExpr}{{\n\t{defaultConstructor}{SetContentExprList}\n}};'

        if struct.protect!=None and struct.protect!='':
            structDefine=f'\n#ifdef {struct.protect}\n{structDefine}\n#endif /*{struct.protect}*/\n'

        SectionStructDefines+=structDefine
        '''
        StructCode[struct] = structDefine
        Struct2Depends[struct ] = depends
        for depend in depends:
            if depend not in depends2Struct:
                depends2Struct[depend] = struct 
            else 
        '''
    
    def processEachFunctionality(parser:VkXmlParser, name:str, func:VkFunctionity):
        pass
    
    # vkcommand
    def GenerateCommandStruct(parser:VkXmlParser, name:str, command:VkCommand):
        global Depends2Code
        CommandDefine =''
        CommandSignation=''
        functionName = GetNewStructName(name)

        CommandInParamList='' 
        CommandExecuteParamList=''

        index=0
        for paramName,param in command.params.items():
            CommandInParam=''
            CommandExecuteParam=''

            typeName = param.type
            if param.type in parser.structTypes:
                typeName = GetDependsStructName(param.type)

            # father handle
            if index == 0 and param.type in parser.handles:
                CommandExecuteParam = 'handle'
            elif paramName == 'pAllocator':
                CommandExecuteParam = f', nullptr'
            else:
                CommandExecuteParam = f', {paramName}'
                if index ==1:
                    CommandInParam= f' {param.getFullType()} {paramName}{param.array}'
                else:
                    CommandInParam = f', {param.getFullType()} {paramName}{param.array}'

            index=index+1

            # add to depends
            if param.type in parser.structTypes:
                GetDependsStructName(paramName)
                
            CommandInParamList += CommandInParam
            CommandExecuteParamList+=CommandExecuteParam
        
        ret = ''
        if command.ret!='' and command.ret!='void':
            ret='return '
        CommandBody = f'ARAssert(DispatchTable.{name}!=nullptr);\n\t\t{ret}DispatchTable.{name}({CommandExecuteParamList});'

        CommandSignation=f'{command.ret} {functionName}( {CommandInParamList})'
        CommandDefine = f'{CommandSignation}{{\n\t\t{CommandBody}\n\t}}'

        if parser.protect!=None:
            CommandDefine = f'#ifdef {parser.protect}\n\t{CommandDefine}\n#endif'
        else:
            CommandDefine='\n\t'+CommandDefine

        Depends2Code[name] = CommandDefine

    def GenerateHandleStruct(parser:VkXmlParser, name:str, handleType:VkHandle):
        global HandleObjectDefines

        newName = GetStructName(name)
        structCommands = ''
        for cmdName, command in handleType.commands.items():
            if cmdName not in Depends2Code:
                print(f'{cmdName} not in depends2code')
                continue
            structCommands+=f'\n{Depends2Code[cmdName]}'
        if structCommands=='':
            return
        
        parent =''
        parameterList=''
        constructorInitializeList=''
        constructorList=''
        if name == 'VkInstance':
            parent = ': public VkInstDispatchTable'
            constructorInitializeList=':VkInstDispatchTable{}'
            structCommands=structCommands.replace('DispatchTable.', 'this->')
        elif name == 'VkPhysicalDevice':
            parent = ': public VkDevDispatchTable'
            constructorInitializeList=':VkDevDispatchTable{}'
            structCommands=structCommands.replace('DispatchTable.', 'GInstanceCommands()->')
            structCommands=structCommands.replace('handle', 'physicalHandle')
        elif name =='VkDevice':
            parentName = GetStructName("VkPhysicalDevice")
            parent = f': public {parentName}'
            constructorInitializeList=f':{parentName}{{}}'
            structCommands=structCommands.replace('DispatchTable.', 'this->')
        else:
            parameterList = f'{handleType.name} handle;'
            constructorList = f'{handleType.name} inHandle'
            constructorInitializeList = f'\n\t: handle{{ inHandle }}'
            constructorInitializeList +=f'\n\t, DispatchTable{{ inDispatchTable }}'

            if handleType.commandDispatch == ECommandType.Instance:
                parameterList += f'\n\tVkInstDispatchTable& DispatchTable;'
                constructorList += f', VkInstDispatchTable& inDispatchTable'
                
            elif handleType.commandDispatch == ECommandType.Device:
                parameterList += f'\n\tVkDevDispatchTable& DispatchTable;'
                constructorList += f', VkDevDispatchTable& inDispatchTable'
            else:
                raise ValueError("execption type")

    
        structDefine = f'struct {newName}{parent}{{\n\t{parameterList}\n\t{newName}({constructorList}){constructorInitializeList}\n\t{{}}{structCommands}\n}};'

        HandleObjectDefines+=f'\n\n{structDefine}'

    # command
    # , typeCallback=GenerateDependsStruct
    parser.foreachFunctionity(processEachFunctionality, commandCallback=GenerateCommandStruct)
    
    # for name, handleType in parser.handles.items():
    instanceHandle = parser.handles['VkInstance']
    visitQueue = deque()
    visitQueue.append(instanceHandle)

    while len(visitQueue) > 0:
        handle = visitQueue.pop()
        GenerateHandleStruct(parser, handle.name, handle)
        for _,child in handle.children.items():
            visitQueue.append(child)


    depends2Struct={}
    
    ''''''
    for struct in StructsDepends:
        if struct in visitQueue:
            visitQueue.remove(struct)
        visitQueue.append(struct)

    StructsDepends={}
    Dup = {}
    while len(visitQueue)>0:
        while len(visitQueue)>0:
            struct = visitQueue.pop()
            if struct not in StructsDepends:
                if struct in parser.structTypes:
                    type = parser.structTypes[struct]
                    if struct not in Dup:
                        GenerateDependsStruct(parser, struct, type)
                        Dup[struct]=1
        StructDefines =SectionStructDefines+StructDefines
        SectionStructDefines=''

        for struct in StructsDepends:
            if struct in visitQueue:
                visitQueue.remove(struct)
            visitQueue.append(struct)
        StructsDepends={}
    

    CommandInfo.write(
f'''
#include"vk_common.h"

PROJECT_NAMESPACE_BEGIN

{StructDefines}

{HandleObjectDefines}

PROJECT_NAMESPACE_END
''')


"""
write vk_extension_defs.h
"""
with open(scriptPath + 'vk_extension_defs.h', mode='w', newline = None) as CommandInfo:
    parser.writeDeclare(CommandInfo)

    
    ExtensionEnumsList=''
    platformsList=''
    definesList=''

    ExtensionStructList=''
    InstanceIndexList =''
    DeviceIndexList =''

    ExtensionStructName ='VulkanExtension'
    ExtensionIndexEnum=f'E{ExtensionStructName}Index'
    ExtensionTypeEnum=f'E{ExtensionStructName}Type'
    ExtensionPlatformEnum=f'E{ExtensionStructName}Platform'

    platformDup={}
    extensionDup={}
    ExtensionCount=0

    def processEachFunctionality(parser:VkXmlParser, name:str, func:VkFunctionity):
        global ExtensionEnumsList,platformsList,definesList
        global ExtensionStructList, ExtensionStructName,platformDup,extensionDup
        global ExtensionCount, InstanceIndexList,DeviceIndexList

        if isinstance(func, VkExtension) and func.type!=None:
            platformEnum = 'all'
            if func.platform!='':
                platformEnum = func.platform
            
            if func.strName =='':
                raise ValueError(f"{func.name} don't have EXTENSION_NAME")

            def getSingleExensionEnumName(extension:str):
                return extension +"_index"

            def getMultiExtensionEnumCompose(requires:str, enumclass:str):
                requires = requires.split(',')
                list =''
                index=0
                for require in requires:
                    if require == '':
                        continue
                    dot=''
                    if index>0:
                        dot=','
                    list += f'{dot} {enumclass}::{getSingleExensionEnumName(require)}'
                    index=index+1
                if index>0:
                    list+=f', {enumclass}::None'
                else:
                    list+=f' {enumclass}::None'
                index=index+1
                final = f'{{{list}}}'
                return final
            
            def WrapWithProtect( str):
                protect = WrapWithProtect.protect
                final = str
                if protect!='' and protect!=None:
                    final = f'\n#ifdef {protect}{str}\n#endif'
                else:
                    final = f'{final}'
                return final
            WrapWithProtect.protect= func.protect

            upper = func.name.upper()
            extensionName = func.strName
            enumName = getSingleExensionEnumName(func.name)

            if func.name not in extensionDup:
                enumItem = f'{ExtensionIndexEnum}::{enumName}'
                extensionDup[func.name] = ExtensionCount
                dot=','
                if ExtensionCount == 0:
                    dot=''

                ExtensionEnumsList+=WrapWithProtect( f'\n\t{dot}{enumName}/*{func.type}*/')
                ExtensionStructList+= WrapWithProtect( f'\n\t\t{dot}{{ (bool){func.name}, {ExtensionTypeEnum}::{func.type}, {enumItem},  {extensionName},'\
                    f' {ExtensionPlatformEnum}::{platformEnum}, {getMultiExtensionEnumCompose(func.requires,ExtensionIndexEnum)} }}')

                #enumItem = f'{WrapWithProtect(f"{enumItem},")}'
                
                if func.type == 'instance':
                    if InstanceIndexList=='':

                        InstanceIndexList+=f'\n\t\t{enumItem}'
                    else:
                        enumItem=f'\n\t\t,{enumItem}'
                        InstanceIndexList+=WrapWithProtect(enumItem)
                elif func.type == 'device':
                    if DeviceIndexList=='':
                        DeviceIndexList+=f'\n\t\t{enumItem}'
                    else:
                        enumItem=f'\n\t\t,{enumItem}'
                        DeviceIndexList+=WrapWithProtect(enumItem)
                else:
                    raise ValueError("execeptional type")

            if func.platform not in platformDup and func.platform!='':
                platformsList +=f'\n\t{func.platform} ,'
                platformDup[func.platform]=1

            if func.name!='':
                customSwtich=f'{upper}_CUSTOM_SWITCH'
                customSwitchExpr = f'#ifndef {customSwtich}\n#define {customSwtich} 1\n#endif'
                extensionDefine = f'#ifdef {func.name}\n{customSwitchExpr}\n#define ENABLE_{upper} {func.name} && {customSwtich} \n#else\n#define ENABLE_{upper} 0\n#endif'
                definesList +=f'\n\n\n{extensionDefine}'
            ExtensionCount=ExtensionCount+1
        pass
    
    parser.foreachFunctionity(processEachFunctionality)
    ExtensionEnumsList+=f'\n\t,Count'
    ExtensionEnumsList+=f'\n\t,None'

    CommandInfo.write(
f'''

#include"vk_common.h"

{definesList}

enum class {ExtensionPlatformEnum}{{
    all ,{platformsList}
}};

enum class {ExtensionIndexEnum}{{{ExtensionEnumsList}
}};

enum class {ExtensionTypeEnum}{{
    instance,
    device
}};

struct {ExtensionStructName}{{
    static constexpr int kMaxInstanceRequires=8;
    bool                    enabled;
    {ExtensionTypeEnum}     type;
    {ExtensionIndexEnum}    index;
    char const*             extensionName;
    {ExtensionPlatformEnum} platform;
    {ExtensionIndexEnum}    requires[kMaxInstanceRequires];
}};
 
inline static {ExtensionStructName} GDefaultExtensions[(int){ExtensionIndexEnum}::Count]={{{ExtensionStructList}
}};
constexpr {ExtensionIndexEnum} kInstanceExtensionIndices[]={{{InstanceIndexList}
}};
constexpr {ExtensionIndexEnum} kDeviceExtensionIndices[]={{{DeviceIndexList}
}};
constexpr int kInstanceExtensionCount = sizeof(kInstanceExtensionIndices) / sizeof({ExtensionIndexEnum});
constexpr int kDeviceExtensionCount = sizeof(kDeviceExtensionIndices) / sizeof({ExtensionIndexEnum});

template<typename Filter, typename Call>
inline void ForeachVulkanExtension(Filter&& filter, Call call){{
    for(int i=0; i<kInstanceExtensionCount; ++i){{
        int index = (int) kInstanceExtensionIndices[i];
        if(filter(GDefaultExtensions[index])){{
            call(GDefaultExtensions[index]);
        }}
    }}
    for(int i=0; i<kDeviceExtensionCount; ++i){{
        int index = (int) kDeviceExtensionIndices[i];
        if(filter(GDefaultExtensions[index])){{
            call(GDefaultExtensions[index]);
        }}
    }}
}}

inline void GetVulkanInstanceExtensionStrings(std::vector<char const*>& extensions){{
    for(int i=0; i<kInstanceExtensionCount; ++i){{
        int index = (int) kInstanceExtensionIndices[i];
        extensions.emplace_back(GDefaultExtensions[index].extensionName);
    }}
}}

inline void GetVulkanDeviceExtensionStrings(std::vector<char const*>& extensions){{
    for(int i=0; i<kDeviceExtensionCount; ++i){{
        int index = (int) kDeviceExtensionIndices[i];
        extensions.emplace_back(GDefaultExtensions[index].extensionName);
    }}
}}

inline {ExtensionStructName}& GetVulkanExtension({ExtensionIndexEnum} Enum){{
    int ienum = (int)Enum;
    ARAssert(ienum<(int){ExtensionIndexEnum}::Count);
    return GDefaultExtensions[ienum];
}}

#define GVulkanExtension(ExtensionName) GetVulkanExtension( {ExtensionIndexEnum}::ExtensionName ## _index )
#define IsVkExtensionEnabled(ExtensionName) GetVulkanExtension( EVulkanExtensionIndex::ExtensionName ## _index ).enabled
''')


"""
write vk_format_defs.h
"""
with open(scriptPath + 'vk_format_defs.h', mode='w', newline = None) as CommandInfo:
    parser.writeDeclare(CommandInfo)

    formatIndexList=''
    formatMappingIndexList=''
    formatCompressList=''
    formatInfoList=''

    index=0
    compressMap={}
    for formatName, format in parser.formats.items():
        dot=', '
        if index ==0:
            dot=''

        indexName = formatName+"_index"
        formatIndexList+=f'\n\t{dot}{indexName}'
        formatMappingIndexList+=f'\n\t\tcase {formatName}: return (int)EVkFormatIndex::{indexName};'

        pack = format.packed
        if pack=='':
            pack='0'

        compress='None'
        if format.isCompressed():
            compress = format.compressed.replace(' ', '_')
            if format.compressed not in compressMap:
                formatCompressList+=f'\n\t{dot}'+compress
                compressMap[format.compressed]=1
 
            

        blockExtend='1,1,1'
        if format.blockExtent!='':
            blockExtend = format.blockExtent

        formatInfoList+=f'\n\t{dot}{{ {formatName}, EFormatCompress::{compress}, {format.blockSize}, {format.texelsPerBlock}, {pack}, {blockExtend}  }}'
        index=index+1

    CommandInfo.write(
f'''
#include"vk_common.h"

PROJECT_NAMESPACE_BEGIN
enum class EVkFormatIndex
{{{formatIndexList}
    , Count
    , Unkown
}};

enum class EFormatCompress: uint8
{{
    None{formatCompressList}
}};

struct VkFormatInfo
{{
    VkFormat format;
    EFormatCompress compress;
    uint8 blockSize;
    uint8 texelsPerBlock;
    uint8 pack;
    uint8 blockWidth;
    uint8 blockHeight;
    uint8 blockDepth;
}};

inline static VkFormatInfo GVkFormats[(uint32)EVkFormatIndex::Count] ={{{formatInfoList}
}};

inline uint32 GetVkFormatIndex(VkFormat format)
{{
    switch(format){{{formatMappingIndexList}
        default: break;
    }}
    return (uint32)EVkFormatIndex::Unkown;
}};

inline VkFormatInfo& GetVkFormatInfo(VkFormat format){{
    uint32 index = GetVkFormatIndex(format);
    ARAssert(index < (uint32)EVkFormatIndex::Count);
    return GVkFormats[index];
}}

inline VkDeviceSize GetImageSize(VkFormat format, uint32 width, uint32 height, uint32 depth=1){{
    return 0;
}}
PROJECT_NAMESPACE_END
''')