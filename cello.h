/*
    Copyright (c) 2023 Brett g Porter
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#pragma once
#define CELLO_H_INCLUDED

/*
BEGIN_JUCE_MODULE_DECLARATION
 ID:               cello
 vendor:           Brett g Porter
 version:          1.7.1
 name:             Cello
 description:      Classes for working with JUCE Value Trees
 website:          https://github.com/bgporter/cello
 license:          MIT
 minimumCppStandard: 17

 dependencies:     juce_core, juce_data_structures
END_JUCE_MODULE_DECLARATION
*/

#include "cello/cello_computed_value.h"
#include "cello/cello_ipc.h"
#include "cello/cello_object.h"
#include "cello/cello_path.h"
#include "cello/cello_query.h"
#include "cello/cello_sync.h"
#include "cello/cello_update_source.h"
#include "cello/cello_value.h"