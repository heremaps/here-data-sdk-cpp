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

#import "MyViewController.h"
#include "gtest/gtest.h"
#import <vector>
#import <mach/mach.h>

@interface MyViewController ()

@property (weak, nonatomic) IBOutlet UILabel *testAppName;

//-------------- CPU & Memory Sampling & Recording ------------------
@property (nonatomic) NSTimeInterval memSamplingPeriod;   //  in second
@property (nonatomic) NSTimeInterval cpuSamplingPeriod;   //  in second
@property (nonatomic) BOOL automaticRecordingEnabled;
@property (nonatomic) BOOL automaticRecordingInProgress;
@end

@implementation MyViewController
{
    int argccopy;
    std::vector<std::unique_ptr<char>> argvcopy;
    BOOL _run_with_global_queue;
    NSString *_memUsageFile;
    NSString *_cpuUsageFile;

    NSString* _documentsDirectory;
    BOOL _stdout_redirected;
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    self.testAppName.text = NSBundle.mainBundle.infoDictionary[@"CFBundleName"];

    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    _documentsDirectory = [paths objectAtIndex:0];

    // Run tests
    self.testDone = NO;
    _run_with_global_queue = NO;
    _automaticRecordingEnabled = NO;
    _automaticRecordingInProgress = NO;

    [self runTest];
    [self waitForTestDone];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)redirectStdout:(NSString*)filename
{
    _stdout_redirected = NO;
    NSString* redirectFile = [NSString stringWithFormat:@"%@/%s", _documentsDirectory, [filename UTF8String]];
    if (std::freopen([redirectFile UTF8String], "w", stdout))
    {
        NSLog(@"Redirect stdout to %s success", [redirectFile UTF8String]);
        _stdout_redirected = YES;
    }
    else
    {
        NSLog(@"Redirect stdout to %s failed", [redirectFile UTF8String]);
    }
}

- (void)runTest
{
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
    // Process any configuration command line arguments
        NSMutableArray *arguments = [NSMutableArray arrayWithArray:[[NSProcessInfo processInfo] arguments]];
        NSMutableArray *toRemoveItems = [NSMutableArray new];
        for (int ii = 1; ii < arguments.count; ii++)
        {
            NSString *item = arguments[ii];
            if ([item compare:@"--unit_test_global_queue"] == NSOrderedSame)
            {
                _run_with_global_queue = YES;
                // Consume this parameter
                [toRemoveItems addObject:item];
            }
            else if ([item compare:@"--automatic_recording"] == NSOrderedSame)
            {
                _automaticRecordingEnabled = YES;
                // Consume this parameter
                [toRemoveItems addObject:item];
            }
            else if ([item hasPrefix:@"--redirectStdout="])
            {
                NSArray *components = [item componentsSeparatedByString:@"="];
                [self redirectStdout:[components lastObject]];
                // Consume this parameter
                [toRemoveItems addObject:item];
            }
            else if ([item hasPrefix:@"--sampling_interval="])
            {
                NSArray *components = [item componentsSeparatedByString:@"="];
                _memSamplingPeriod = [components[1] doubleValue];
                _cpuSamplingPeriod = _memSamplingPeriod;
                // Consume this parameter
                [toRemoveItems addObject:item];
            }
        }

        for (NSString *s in toRemoveItems)
        {
            [arguments removeObjectIdenticalTo:s];
        }

        int argc = (int)arguments.count;

        // Add "--gtest_output=xml:<Documents directory>/output.xml" to command arguments
        NSString* filename = [[NSString alloc] initWithFormat:@"%s%@%s", "--gtest_output=xml:", _documentsDirectory, "/output.xml"];

        int new_argc = argc + 1;
        std::vector<std::unique_ptr<char>> new_argv_aux(new_argc); // automatically destroyed
        if (_run_with_global_queue)
        {
            argccopy = new_argc;
            argvcopy.resize(argccopy);
        }
        for (int ii = 0; ii < argc; ++ii) {
            NSString* arg = (NSString*)[arguments objectAtIndex:ii];
            new_argv_aux[ii].reset(new char[arg.length + 1]);
            strcpy(new_argv_aux[ii].get(), [arg UTF8String]);
            if (_run_with_global_queue)
            {
                argvcopy[ii].reset(new char[arg.length + 1]);
                strcpy(argvcopy[ii].get(), [arg UTF8String]);
            }
        }
        new_argv_aux[argc].reset(new char[filename.length + 1]);
        strcpy(new_argv_aux[argc].get(), [filename UTF8String]);
        if (_run_with_global_queue)
        {
            argvcopy[argc].reset(new char[filename.length + 1]);
            strcpy(argvcopy[argc].get(), [filename UTF8String]);
        }

        // Now the actual double pointer is extracted from here
        std::vector<char*> new_argv(new_argv_aux.size());
        for (size_t ii = 0; ii < new_argv_aux.size(); ++ii) {
            new_argv[ii] = new_argv_aux[ii].get(); // soft-reference
            NSLog(@"   argc %zu: %s", ii, new_argv[ii]);
        } // last element is null from std::unique_ptr constructor


        testing::InitGoogleTest(&new_argc, new_argv.data());

        int result = RUN_ALL_TESTS();
        if (!_run_with_global_queue)
        {
            // Create Tests Completed Indicator File
            NSString* file = [_documentsDirectory stringByAppendingPathComponent:@"Tests_Completed_Indicator"];
            NSLog(@"     Tests_Completed_Indicator file: %s", [file UTF8String]);
            [[NSString stringWithFormat:@"Gtest result: %d",result] writeToFile:file atomically:YES encoding:NSStringEncodingConversionAllowLossy error:nil];
            self.testDone = true;
        }
        else
        {
            NSFileManager *fileMgr = [NSFileManager defaultManager];
            filename = [_documentsDirectory stringByAppendingString:@"/output.xml"];
            if ( [fileMgr fileExistsAtPath:filename] )
            {
                [fileMgr moveItemAtPath:filename toPath:[_documentsDirectory stringByAppendingString:@"/output_main.xml"] error:nil];
            }

            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1 * NSEC_PER_SEC)),
                           dispatch_get_global_queue(QOS_CLASS_DEFAULT, 0), ^(){
                int argc2 = argccopy;
                std::vector<char*> argv2(argc2);
                for (size_t ii = 0; ii < argc2; ii++)
                {
                    argv2[ii] = argvcopy[ii].get();
                }

                testing::InitGoogleTest(&argc2, argv2.data());
                int result = RUN_ALL_TESTS();

                // Create Tests Completed Indicator File
                NSString* file = [_documentsDirectory stringByAppendingPathComponent:@"Tests_Completed_Indicator"];
                NSLog(@"     Tests_Completed_Indicator file: %s", [file UTF8String]);
                [[NSString stringWithFormat:@"Gtest result: %d",result] writeToFile:file atomically:YES encoding:NSStringEncodingConversionAllowLossy error:nil];

                if ( [fileMgr fileExistsAtPath:filename] )
                {
                    [fileMgr moveItemAtPath:filename toPath:[_documentsDirectory stringByAppendingString:@"/output_global.xml"] error:nil];
                    [fileMgr moveItemAtPath:[_documentsDirectory stringByAppendingString:@"/output_main.xml"] toPath:filename error:nil];
                }

                self.testDone = true;
            });
        }


    });
}

- (void)waitForTestDone
{
    if (!_memSamplingPeriod) {
        _memSamplingPeriod = 60;
        _cpuSamplingPeriod = 60;
    }

    if (( _automaticRecordingEnabled ) && (!_automaticRecordingInProgress)) {
        [self deleteOldSampleFiles];
        [self createNewSampleFiles];
        [self samplingMemoryUsage];
        [self samplingCPUUsage];
        _automaticRecordingInProgress = YES;
    }

    // Poll every second until we get the user granted permissions we require
    if (!self.testDone) {
        NSLog(@"Waiting for test to finish");
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(self.memSamplingPeriod * NSEC_PER_SEC)), dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
            [self waitForTestDone];
        });
    } else {
        dispatch_after( dispatch_time(DISPATCH_TIME_NOW, (int64_t)(10 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
            NSLog(@"Exit app");

            if (_stdout_redirected)
            {
                fclose(stdout);
            }

            exit(0);
        });
    }
}

// See http://liam.flookes.com/wp/2012/05/03/finding-ios-memory/ for comparisons of ways to measure memory.
//
// See https://stackoverflow.com/questions/787160/programmatically-retrieve-memory-usage-on-iphone for how to retrieve
// resident memory (or real memory as reported by the instruments activity monitor tool).
//
// See https://stackoverflow.com/questions/13437365/what-is-resident-and-dirty-memory-of-ios for explanation
// of what resident memory is.
//
// Resident memory includes memory we are not using so to get a more accurate picture of how much memory we are
// actually using we need to attempt to purge resident memory - see [UTAHelper purgeResidentMemory]
//
- (NSDictionary*)memoryInfo
{
    static long lastUsed = 0;

    NSUInteger memoryTotalBytes = -1; //Total number of bytes being used
    NSInteger memoryMB = -1; //Floor of the total number of mb being used
    NSInteger memoryKB = -1;//Remainder kb being used

    // Get resident memory used
    struct mach_task_basic_info info;
    mach_msg_type_number_t size = MACH_TASK_BASIC_INFO_COUNT;
    kern_return_t kerr = task_info(mach_task_self(),
                                   MACH_TASK_BASIC_INFO,
                                   (task_info_t)&info,
                                    &size);

    if (kerr == KERN_SUCCESS ) {
        memoryTotalBytes = info.resident_size;
        memoryMB = (long)(info.resident_size / 1048576);
        memoryKB = (long)((info.resident_size - (memoryMB * 1048576)) / 1024);
        lastUsed = (long)(info.resident_size / 1024);
    }

    // Get system free memory
    // Taken from https://stackoverflow.com/questions/5012886/determining-the-available-amount-of-ram-on-an-ios-device/8540665#8540665
    // Comment out for now since does not work on 64bit devices
    //mach_port_t host_port = mach_host_self();
    //mach_msg_type_number_t host_size = sizeof(vm_statistics_data_t) / sizeof(integer_t);
    //vm_size_t pagesize;
    //vm_statistics_data_t vm_stat;
    //host_page_size(host_port, &pagesize);
    //(void) host_statistics(host_port, HOST_VM_INFO, (host_info_t)&vm_stat, &host_size);
    //NSInteger freeMemory = (vm_stat.free_count * pagesize) / 1048576.0;

    return [[NSDictionary alloc] initWithObjectsAndKeys:
        @(memoryTotalBytes), @"used-total-bytes",
        @(memoryMB) , @"used-mb",
        @(memoryKB), @"used-kb",
        //@(freeMemory), @"free",
        nil];
}

// Delete old CPU & Memory status result files
- (void)deleteOldSampleFiles
{
    NSFileManager  *manager = [NSFileManager defaultManager];

    // get the apps tmp directory
    NSString *tmpDirectory = [NSString stringWithFormat:@"%@/%@", [manager temporaryDirectory].path, @"Stability" ];

    // grab all the files in the tmp dir
    NSArray *allFiles = [manager contentsOfDirectoryAtPath:tmpDirectory error:nil];

    // filter the array for only sqlite files
    NSPredicate *fltr = [NSPredicate predicateWithFormat:@"self ENDSWITH 'usage.txt'"];
    NSArray *oldSampleFiles = [allFiles filteredArrayUsingPredicate:fltr];

    // use fast enumeration to iterate the array and delete the files
    for (NSString *sampleFile in oldSampleFiles)
    {
        NSError *error = nil;
        [manager removeItemAtPath:[tmpDirectory stringByAppendingPathComponent:sampleFile] error:&error];
        NSAssert(!error, @"Assertion: SQLite file deletion shall never throw an error.");
    }
}

// Create new CPU & Memory status result files
- (void)createNewSampleFiles
{
    // get the apps tmp directory
    NSFileManager  *manager = [NSFileManager defaultManager];

    NSString *tmpDirectory = [NSString stringWithFormat:@"%@/%@", [manager temporaryDirectory].path, @"Stability"];
    NSLog(@"Will save stability test summary to directory: %@", tmpDirectory);

    _memUsageFile = [NSString stringWithFormat:@"%@/mem_usage.txt", tmpDirectory];
    _cpuUsageFile = [NSString stringWithFormat:@"%@/cpu_usage.txt", tmpDirectory];

    [[NSFileManager defaultManager] createDirectoryAtPath:tmpDirectory withIntermediateDirectories:YES attributes:nil error:nil];
}

#pragma mark - Memory Usage

- (void)samplingMemoryUsage
{
    NSLog(@"Resident Memory: %@", [self memoryUsed]);

    __weak id weakSelf = self;
    dispatch_time_t popTime = dispatch_time(DISPATCH_TIME_NOW, (int64_t)(self.memSamplingPeriod * NSEC_PER_SEC));
    dispatch_after(popTime, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(void){
        [weakSelf samplingMemoryUsage];
    });
}

- (NSString*)memoryUsed
{
    NSDictionary *memoryInfo = [self memoryInfo];

    uint32_t totalBytes = [(NSNumber*)[memoryInfo valueForKey:@"used-total-bytes"] unsignedIntValue];
    long usedMB = [(NSNumber*)[memoryInfo valueForKey:@"used-mb"] longValue];
    long usedKB = [(NSNumber*)[memoryInfo valueForKey:@"used-kb"] longValue];

    [self writeMemoryUsedToTextFile:totalBytes];

    return [NSString stringWithFormat:@"%3ld MB %4ld KB, ", usedMB, usedKB];
}

- (void)writeMemoryUsedToTextFile:(uint32_t)memUsed
{
    NSString *data = [NSString stringWithFormat:@"%u\n", (uint32_t)memUsed];
    //save content to the tmp directory
    NSFileManager *fileManager = [NSFileManager defaultManager];
    if ( ![fileManager fileExistsAtPath:_memUsageFile] ) {
        NSError* __autoreleasing error;
        [data writeToFile:_memUsageFile atomically:NO encoding:NSStringEncodingConversionAllowLossy error:&error];
    }
    else {
        NSFileHandle *myHandle = [NSFileHandle fileHandleForWritingAtPath:_memUsageFile];
        [myHandle seekToEndOfFile];
        [myHandle writeData:[data dataUsingEncoding:NSUTF8StringEncoding]];
    }
}


#pragma mark - CPU Usage

- (void)samplingCPUUsage
{
    float cpuUsage = [self cpuUsed]/2;
    NSLog(@"Resident CPU: %@", [NSString stringWithFormat:@"%1.1f %c", cpuUsage, '%']);

    [self writeCPUUsageToTextFile:cpuUsage];

    __weak id weakSelf = self;
    dispatch_time_t popTime = dispatch_time(DISPATCH_TIME_NOW, (int64_t)(self.cpuSamplingPeriod * NSEC_PER_SEC));
    dispatch_after(popTime, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(void){
        [weakSelf samplingCPUUsage];
    });
}

- (float)cpuUsed
{
    kern_return_t kr;
    task_info_data_t tinfo;
    mach_msg_type_number_t task_info_count;

    task_info_count = TASK_INFO_MAX;
    kr = task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)tinfo, &task_info_count);
    if (kr != KERN_SUCCESS) {
        return -1;
    }

    //    task_basic_info_t      basic_info;
    thread_array_t         thread_list;
    mach_msg_type_number_t thread_count;

    thread_info_data_t     thinfo;
    mach_msg_type_number_t thread_info_count;

    thread_basic_info_t basic_info_th;
    //    uint32_t stat_thread = 0; // Mach threads

    //    basic_info = (task_basic_info_t)tinfo;

    // get threads in the task
    kr = task_threads(mach_task_self(), &thread_list, &thread_count);
    if (kr != KERN_SUCCESS) {
        return -1;
    }
    //    if (thread_count > 0)
    //        stat_thread += thread_count;

    long tot_sec = 0;
    long tot_usec = 0;
    float tot_cpu = 0;
    int j;

    for (j = 0; j < thread_count; j++)
    {
        thread_info_count = THREAD_INFO_MAX;
        kr = thread_info(thread_list[j], THREAD_BASIC_INFO,
                         (thread_info_t)thinfo, &thread_info_count);
        if (kr != KERN_SUCCESS) {
            return -1;
        }

        basic_info_th = (thread_basic_info_t)thinfo;

        if (!(basic_info_th->flags & TH_FLAGS_IDLE)) {
            tot_sec = tot_sec + basic_info_th->user_time.seconds + basic_info_th->system_time.seconds;
            tot_usec = tot_usec + basic_info_th->system_time.microseconds + basic_info_th->system_time.microseconds;
            tot_cpu = tot_cpu + basic_info_th->cpu_usage / (float)TH_USAGE_SCALE * 100.0;
        }
    } // for each thread

    kr = vm_deallocate(mach_task_self(), (vm_offset_t)thread_list, thread_count * sizeof(thread_t));
    assert(kr == KERN_SUCCESS);
    return tot_cpu;
}

- (void)writeCPUUsageToTextFile:(float)cpuUsage
{
    NSString *data = [NSString stringWithFormat:@"%.2f\n", cpuUsage];
    //save content to the tmp directory
    NSFileManager *fileManager = [NSFileManager defaultManager];
    if ( ![fileManager fileExistsAtPath:_cpuUsageFile] ) {
        NSError* __autoreleasing error;
        [data writeToFile:_cpuUsageFile atomically:NO encoding:NSStringEncodingConversionAllowLossy error:&error];
    }
    else {
        NSFileHandle *myHandle = [NSFileHandle fileHandleForWritingAtPath:_cpuUsageFile];
        [myHandle seekToEndOfFile];
        [myHandle writeData:[data dataUsingEncoding:NSUTF8StringEncoding]];
    }
}
@end
