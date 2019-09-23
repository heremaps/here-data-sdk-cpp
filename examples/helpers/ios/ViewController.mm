/*
 * Copyright (C) 2019 HERE Europe B.V.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 * License-Filename: LICENSE
 */

#import "ViewController.h"

#include "example.h"

@interface ViewController ()

@property(weak, nonatomic) IBOutlet UILabel* _testAppLabel;
@property(weak, nonatomic) IBOutlet UILabel* _executionStatusLabel;

@end

@implementation ViewController

- (void)viewDidLoad {
  [super viewDidLoad];

  self._testAppLabel.text = NSBundle.mainBundle.infoDictionary[@"CFBundleName"];
  self._executionStatusLabel.text = @"example is running...";
  self._executionStatusLabel.textColor = [UIColor darkGrayColor];
  [self runExampleProgram];
}

- (void)runExampleProgram {
  // Run example in background thread to not block the UI
  __weak ViewController* weakSelf = self;
  dispatch_async(
      dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), ^{
        __block int result = RunExample();

        // Display the results in the main thread
        dispatch_async(dispatch_get_main_queue(), ^{
          __strong ViewController* strongSelf = weakSelf;
          if (strongSelf) {
            // Change the UI Label apperance based on the execution result
            NSString* resultStr =
                NSBundle.mainBundle.infoDictionary
                    [result == 0 ? @"OlpSdkSuccessfulExampleExecutionString"
                                 : @"OlpSdkFailingExampleExecutionString"];
            NSLog(@"Example run result: %@", resultStr);

            UIColor* textColor =
                result == 0 ? [UIColor blackColor] : [UIColor redColor];

            strongSelf._executionStatusLabel.text = resultStr;
            strongSelf._executionStatusLabel.textColor = textColor;
            strongSelf._executionStatusLabel.font =
                [UIFont boldSystemFontOfSize:20.f];

          } else {
            NSLog(@"ERROR: ViewController is no longer reachable!");
          }
        });
      });
}

@end
