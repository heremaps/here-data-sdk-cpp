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

#include "../example.h"

#import <vector>

@interface ViewController ()

@property (weak, nonatomic) IBOutlet UILabel *testAppName;

@end

@implementation ViewController
{
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    self.testAppName.text = NSBundle.mainBundle.infoDictionary[@"CFBundleName"];
    [self runExampleProgram];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)runExampleProgram
{
    dispatch_async(dispatch_get_main_queue(), ^{
        // Process command line arguments and adds file parameter, which will be written by OLP
        NSArray *arguments = [[NSProcessInfo processInfo] arguments];
        const int argc = (int)arguments.count;
        const int new_argc = argc + 2;
        std::vector<const char*> new_argv(new_argc);
        for (int i = 0; i < argc; ++i) {
            NSString *arg = (NSString*)[arguments objectAtIndex:i];
            new_argv[i] = [arg UTF8String];
        }

        new_argv[argc] = "--file";

        NSString *path = [[NSBundle mainBundle] pathForResource:@"dummy" ofType:@"txt"];
        NSString *fileName = [NSURL fileURLWithPath:path].path;
        new_argv[argc+1] = [fileName UTF8String];

        runExample(new_argc, new_argv.data());
    });
}

@end
