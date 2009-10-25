/*
    Copyright (c) 2007-2009 Takashi Kitao
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
    `  notice, this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright
    `  notice, this list of conditions and the following disclaimer in the
    `  documentation and/or other materials provided with the distribution.

    3. The name of the author may not be used to endorse or promote products
    `  derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include "test.h"


void pgMsgTest()
{
    /*
        pgMsg()
        template<class T> T getParam(u8 index)
        template<class T> void setParam(u8 index, T param)
    */
    {
        pgMsg<2> msg1;
        pgAssertThrow(msg1.getParam<u8>(0), pgMsg<2>::ExceptionInvalidCall);
        pgAssertThrow(msg1.getParam<s16>(1), pgMsg<2>::ExceptionInvalidCall);
        pgAssertThrow(msg1.getParam<r32>(2), pgMsg<2>::ExceptionInvalidArgument);

        msg1.setParam<s8>(0, -123);
        pgAssert(msg1.getParam<s8>(0) == -123);
        pgAssertThrow(msg1.getParam<s16>(0), pgMsg<2>::ExceptionInvalidType);

        msg1.setParam<r32>(1, 1.234f);
        pgAssert(isEqual(msg1.getParam<r32>(1), 1.234f));
        pgAssertThrow(msg1.getParam<r64>(0), pgMsg<2>::ExceptionInvalidType);

        pgMsg<2> msg2 = msg1;
        pgAssert(msg2.getParam<s8>(0) == -123);
        pgAssert(isEqual(msg2.getParam<r32>(1), 1.234f));
        pgAssertThrow(msg2.getParam<u64>(2), pgMsg<2>::ExceptionInvalidArgument);

        msg2.setParam<s16>(0, -12345);
        pgAssert(msg2.getParam<s16>(0) == -12345);
        pgAssertThrow(msg2.getParam<u8>(0), pgMsg<2>::ExceptionInvalidType);

        msg2.setParam<u32>(1, 54321);
        pgAssert(msg2.getParam<u32>(1) == 54321);
        pgAssertThrow(msg2.getParam<s16>(1), pgMsg<2>::ExceptionInvalidType);

        msg1 = msg2;
        pgAssert(msg2.getParam<s16>(0) == -12345);
        pgAssert(msg2.getParam<u32>(1) == 54321);
    }
}
