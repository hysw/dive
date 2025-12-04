/*
 Copyright 2025 Google LLC

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/

#include "absl/flags/flag.h"
#include "ui/main.h"

ABSL_FLAG(std::string, test_screenshot, "", "Take screenshot after load, then exit.");
ABSL_FLAG(bool, test_exit_after_load, false, "Test file loading");

int main(int argc, char** argv)
{
    DiveUIMain runner(argc, argv);
    runner.SetOptions(DiveUIMain::TestOptions{
    .exit_after_load = absl::GetFlag(FLAGS_test_exit_after_load),
    .screenshot = absl::GetFlag(FLAGS_test_screenshot),
    });
    return runner.Run();
};
