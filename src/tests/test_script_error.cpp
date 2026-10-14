/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file test_script_error.cpp Tests for command error mappings exposed to scripts. */

#include "../stdafx.h"

#include "../3rdparty/catch2/catch.hpp"

#include "../game/game_instance.hpp"
#include "../script/api/script_error.hpp"
#include "../script/api/script_vehicle.hpp"
#include "../script/squirrel.hpp"
#include "table/strings.h"

#include "../safeguards.h"

extern void SQAI_RegisterAll(Squirrel &engine);
extern void SQGS_RegisterAll(Squirrel &engine);

/** Keep the active script instance and allocator available while registering an API. */
class TestScriptErrorController : public ScriptObject {
public:
	GameInstance game{};
	ActiveInstance active{game};
	Squirrel engine{"test"};
	ScriptAllocatorScope scope{&engine};
};

TEST_CASE("Scripts recognize failure to find a depot", "[script]")
{
	TestScriptErrorController controller;

	/* Each API must register its own mappings, independently of earlier tests. */
	ScriptError::RegisterErrorMap(STR_ERROR_UNABLE_TO_FIND_ROUTE_TO, ScriptError::ERR_UNKNOWN);
	ScriptError::RegisterErrorMap(STR_ERROR_UNABLE_TO_FIND_LOCAL_DEPOT, ScriptError::ERR_UNKNOWN);
	ScriptError::RegisterErrorMap(STR_ERROR_UNABLE_TO_FIND_LOCAL_HANGAR, ScriptError::ERR_UNKNOWN);

	SECTION("AI") { SQAI_RegisterAll(controller.engine); }
	SECTION("Game Script") { SQGS_RegisterAll(controller.engine); }

	CHECK(ScriptError::StringToError(STR_ERROR_UNABLE_TO_FIND_ROUTE_TO) == ScriptVehicle::ERR_VEHICLE_CANNOT_SEND_TO_DEPOT);
	CHECK(ScriptError::StringToError(STR_ERROR_UNABLE_TO_FIND_LOCAL_DEPOT) == ScriptVehicle::ERR_VEHICLE_CANNOT_SEND_TO_DEPOT);
	CHECK(ScriptError::StringToError(STR_ERROR_UNABLE_TO_FIND_LOCAL_HANGAR) == ScriptVehicle::ERR_VEHICLE_CANNOT_SEND_TO_DEPOT);
}
