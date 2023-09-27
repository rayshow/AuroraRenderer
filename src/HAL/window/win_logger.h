#pragma once

#include"../common/stdout_logger.h"

class WindowLogger: public StdoutLogger {};
using Logger = WindowLogger;