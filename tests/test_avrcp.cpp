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

// playItem and changePath with a callback: a SIL that implements only the variants without one keeps working, the
// callback getting their result at once.

#include <glib.h>

#include <string>

#include "bluetooth-sil-api.h"

namespace {

class StubAvrcp : public BluetoothAvrcpProfile
{
public:
	void supplyMediaMetaData(BluetoothAvrcpRequestId, const BluetoothMediaMetaData &, BluetoothResultCallback) override {}
	void supplyMediaPlayStatus(BluetoothAvrcpRequestId, const BluetoothMediaPlayStatus &, BluetoothResultCallback) override {}
	void notifyMediaPlayStatus(const BluetoothMediaPlayStatus &, BluetoothResultCallback) override {}
};

// A SIL built before the callback variants: it has only these two.
class OldAvrcp : public StubAvrcp
{
public:
	using StubAvrcp::playItem;
	using StubAvrcp::changePath;

	BluetoothError playItem(const std::string &itemPath) override
	{
		lastPath = itemPath;
		++calls;
		return result;
	}

	BluetoothError changePath(const std::string &itemPath) override
	{
		lastPath = itemPath;
		++calls;
		return result;
	}

	BluetoothError result = BLUETOOTH_ERROR_NONE;
	std::string lastPath;
	int calls = 0;
};

struct Answer
{
	int calls = 0;
	BluetoothError error = BLUETOOTH_ERROR_PARAM_INVALID;

	BluetoothResultCallback callback()
	{
		return [this](BluetoothError e) {
			++calls;
			error = e;
		};
	}
};

} // namespace

static void test_old_sil_answers_at_once()
{
	OldAvrcp sil;
	BluetoothAvrcpProfile &profile = sil;

	for (BluetoothError result : { BLUETOOTH_ERROR_NONE, BLUETOOTH_ERROR_AVRCP_ITEM_NOT_PLAYABLE })
	{
		sil.result = result;
		Answer answer;
		profile.playItem("/item2", answer.callback());
		g_assert_cmpint(answer.calls, ==, 1);
		g_assert_cmpint(answer.error, ==, result);
		g_assert(sil.lastPath == "/item2");
	}

	for (BluetoothError result : { BLUETOOTH_ERROR_NONE, BLUETOOTH_ERROR_AVRCP_NOT_A_FOLDER })
	{
		sil.result = result;
		Answer answer;
		profile.changePath("/item1", answer.callback());
		g_assert_cmpint(answer.calls, ==, 1);
		g_assert_cmpint(answer.error, ==, result);
		g_assert(sil.lastPath == "/item1");
	}

	// No callback: the request is still made.
	sil.calls = 0;
	profile.playItem("/item3", BluetoothResultCallback());
	profile.changePath("/item4", BluetoothResultCallback());
	g_assert_cmpint(sil.calls, ==, 2);
	g_assert(sil.lastPath == "/item4");
}

static void test_no_browsing_is_unsupported()
{
	StubAvrcp sil;
	BluetoothAvrcpProfile &profile = sil;
	Answer play, change;
	profile.playItem("/item2", play.callback());
	profile.changePath("/item1", change.callback());
	g_assert_cmpint(play.calls, ==, 1);
	g_assert_cmpint(play.error, ==, BLUETOOTH_ERROR_UNSUPPORTED);
	g_assert_cmpint(change.calls, ==, 1);
	g_assert_cmpint(change.error, ==, BLUETOOTH_ERROR_UNSUPPORTED);
}

int main(int argc, char **argv)
{
	g_test_init(&argc, &argv, nullptr);

	g_test_add_func("/avrcp/old-sil-answers-at-once", test_old_sil_answers_at_once);
	g_test_add_func("/avrcp/no-browsing-is-unsupported", test_no_browsing_is_unsupported);

	return g_test_run();
}
