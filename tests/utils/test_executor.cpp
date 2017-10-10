/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Product name: redemption, a FLOSS RDP proxy
   Copyright (C) Wallix 2010
   Author(s): Christophe Grosjean, Javier Caverni, Meng Tan
   Based on xrdp Copyright (C) Jay Sorg 2004-2010

   Unit test to rect object

*/

#define RED_TEST_MODULE TestExecutor
#include "system/redemption_unit_tests.hpp"

#include "utils/executor.hpp"

using R = ExecutorResult;
using A = ExecutorActionContext;
using T = ExecutorTimeoutContext;

RED_AUTO_TEST_CASE(TestExecutor)
{
    Executor executor;
    executor.initial_executor("1")
      .on_action([](A ctx){ return ctx.exit_on_success(); })
      .on_timeout([](T ctx) { return ctx.exit_on_success(); })
      .on_exit([](A ctx, bool) { return ctx.exit_on_success(); })
    ;

    executor.exec();

    executor.initial_executor("1", 1, 2)
      .on_action([](A ctx, int, int){ return ctx.exit_on_success(); })
      .on_timeout([](T ctx, int, int) { return ctx.exit_on_success(); })
      .on_exit([](A ctx, bool, int, int) { return ctx.exit_on_success(); })
    ;

    executor.exec();
}
