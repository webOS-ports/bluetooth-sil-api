// Copyright (c) 2026 Herman van Hazendonk <github.com@herrie.org>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include <glib.h>

#include <stdexcept>
#include <string>

#include "bluetooth-sil-api.h"

template<class P>
static bool throws(const P &property)
{
	try
	{
		property.template getValue<uint32_t>();
	}
	catch (const std::logic_error &)
	{
		return true;
	}
	return false;
}

static void test_property_values(void)
{
	BluetoothProperty name(BluetoothProperty::Type::NAME, std::string("TouchPad"));
	std::string text = "unchanged";
	g_assert(name.tryGetValue(text));
	g_assert(text == "TouchPad");
	g_assert(name.getValue<std::string>() == "TouchPad");

	// Another type: false, the value is left alone, and getValue still throws.
	uint32_t number = 7;
	g_assert(!name.tryGetValue(number));
	g_assert(number == 7);
	g_assert(throws(name));

	// No value at all: a default property, and one with only a type (as SIL error paths pass back).
	g_assert(!BluetoothProperty().tryGetValue(number));
	g_assert(!BluetoothProperty(BluetoothProperty::Type::CLASS_OF_DEVICE).tryGetValue(number));
	g_assert(throws(BluetoothProperty(BluetoothProperty::Type::CLASS_OF_DEVICE)));

	// An enum is not its underlying type.
	BluetoothProperty type(BluetoothProperty::Type::TYPE_OF_DEVICE, BLUETOOTH_DEVICE_TYPE_BREDR);
	g_assert(!type.tryGetValue(number));

	BluetoothProperty copy = BluetoothProperty(BluetoothProperty::Type::CLASS_OF_DEVICE, (uint32_t) 0x5a020c);
	g_assert(copy.tryGetValue(number));
	g_assert(number == 0x5a020c);
}

static void test_map_and_avrcp_values(void)
{
	uint16_t offset = 0;
	BluetoothMapProperty start(BluetoothMapProperty::Type::STARTOFFSET, (uint16_t) 20);
	g_assert(start.tryGetValue(offset));
	g_assert(offset == 20);
	uint8_t length = 3;
	g_assert(!start.tryGetValue(length));
	g_assert(length == 3);
	g_assert(throws(start));
	g_assert(!BluetoothMapProperty(BluetoothMapProperty::Type::MAXCOUNT).tryGetValue(offset));

	BluetoothPlayerApplicationSettingsProperty repeat(BluetoothPlayerApplicationSettingsProperty::Type::REPEAT,
							  REPEAT_ALL_TRACKS);
	BluetoothPlayerApplicationSettingsRepeat mode = REPEAT_OFF;
	g_assert(repeat.tryGetValue(mode));
	g_assert(mode == REPEAT_ALL_TRACKS);
	BluetoothPlayerApplicationSettingsShuffle shuffle = SHUFFLE_OFF;
	g_assert(!repeat.tryGetValue(shuffle));
	g_assert(shuffle == SHUFFLE_OFF);
	g_assert(throws(repeat));
}

int main(int argc, char **argv)
{
	g_test_init(&argc, &argv, nullptr);

	g_test_add_func("/properties/values", test_property_values);
	g_test_add_func("/properties/map-and-avrcp", test_map_and_avrcp_values);

	return g_test_run();
}
