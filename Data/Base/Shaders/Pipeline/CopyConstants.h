#pragma once

#include "../Common/ConstantBufferMacros.h"
#include "../Common/Platforms.h"

CONSTANT_BUFFER(plCopyConstants, 3)
{
  INT2(Offset);
};
