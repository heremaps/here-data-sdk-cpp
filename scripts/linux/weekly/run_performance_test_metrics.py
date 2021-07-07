#!/usr/bin/env python3
#
# Copyright (C) 2021 HERE Europe B.V.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0
# License-Filename: LICENSE

# Script will run olp-cpp-sdk-performance-tests and psrecord with CPU/RAM metrics measurements

import argparse
import builtins
from collections import namedtuple
import jq
import json
from math import ceil
import os
import pprint
import psutil
from re import findall
import requests
from statistics import mean
import shutil
import subprocess
import logging

logging.basicConfig(level=logging.DEBUG)
# 'name' - name of the statistic, e.g. Network, IO
# 'metrics' - appropriate metrics for this statistic
MetricsInfo = namedtuple('MetricsInfo', 'name metrics')

# 'name' - full test name
# 'repeat' - number of times this test should be repeated
# 'metric_infos' - array of MetricsInfo objects
TestInfo = namedtuple('TestInfo', 'name repeat metric_infos')

WORK_DIR = os.getcwd()
DISABLE_UPLOAD_TO_ARTIFACTORY = False


# Use this function to easily switch this script for local work
def setup_for_local_usage():
    # Disable uploading anything to Artifactory
    global DISABLE_UPLOAD_TO_ARTIFACTORY
    DISABLE_UPLOAD_TO_ARTIFACTORY = True
    # Push some arbitrary distinctive job ID
    os.environ['CI_JOB_ID'] = '101010101010101'
    os.environ['ARTIF_BOT'] = ''
    os.environ['ARTIF_BOT_PWD'] = ''


# Purposely shadows built-in print
def print(output):
    builtins.print(output, flush=True)


# Helper to run commands
def shell(command):
    return subprocess.check_output(command, shell=True, env=os.environ).decode('utf-8').strip()


def parse_metric_infos(json):
    metric_infos = []
    for metric_info_json in json:
        name = metric_info_json['name']
        metrics = list(metric_info_json['metrics'])
        metric_infos.append(MetricsInfo(name, metrics))
    return metric_infos


def parse_test_info(json):
    name = json['name']
    repeat = json['repeat']
    metric_infos = parse_metric_infos(json['metric_infos'])
    return TestInfo(name, repeat, metric_infos)


# Loads information about tests from the provided JSON file
def load_tests(path):
    with open(path) as file:
        json_tests = json.load(file)

    test_infos = []
    for test_info_json in json_tests:
        test_infos.append(parse_test_info(test_info_json))
    return test_infos

# Extracts information about current CPU, core count, speed, total RAM amount and updates fields in the report
def update_system_info(tests_file):
    artifactory_host = os.environ['ARTIFACTORY_HOST']

    cpu_model = shell("cat /proc/cpuinfo | grep 'name' | uniq | cut -d':' -f2")
    cpu_speed = shell("cat /proc/cpuinfo | grep 'cpu MHz' | uniq | cut -d':' -f2").split('\n')[0] + ' MHz'
    cpu_count = os.cpu_count()
    total_ram = str(ceil(psutil.virtual_memory().total / (1024.0 ** 3))) + ' GB'

    tests_file = os.path.abspath(tests_file)
    shell(f"sed -i -e \"s/sed_cpu_model/{cpu_model}/g\" {tests_file}")
    shell(f"sed -i -e \"s/sed_mhz/{cpu_speed}/g\" {tests_file}")
    shell(f"sed -i -e \"s/sed_n_core/{cpu_count}/g\" {tests_file}")
    shell(f"sed -i -e \"s/sed_memory_total/{total_ram}/g\" {tests_file}")
    shell(f"sed -i -e \"s/sed_artifactory_host/{artifactory_host}/g\" {tests_file}")


# Helper to fix field names
def fix_bytes(src):
    if src.find('bytes'):
        return src.replace('bytes', 'mb')
    return src


# Constructs full test run command
def get_test_run_command(exec_file, test_name, repeat_num, report_format, output):
    test_executable = os.path.abspath(exec_file)
    if not os.path.isfile(test_executable):
        raise ValueError(f'Invalid path to the test executable file, path={test_executable}')

    return [test_executable, f'--gtest_filter={test_name}', f'--gtest_repeat={repeat_num}',
            f'--gtest_output={report_format}:{output}']


# Runs 'psrecord' tool for each test and collect CPU and RAM usage metrics
def calculate_cpu_and_memory(test_exec, test_infos, collected_metrics):
    for test_info in test_infos:
        test_run_command = get_test_run_command(test_exec, test_info.name, test_info.repeat, 'xml', './reports/output.xml')

        print(f'{test_info.name} measuring ... ################################################')

        with open('./log.txt', 'w') as log_file:
            test_process = subprocess.Popen(test_run_command, stdout=log_file, stderr=subprocess.STDOUT)
            print(f'## Test started as process : {test_process.pid} ##')

            psrecord_output_file = './recording.txt'
            os.system(f'psrecord {test_process.pid} --interval 0.5 --log {psrecord_output_file} --include-children')
            print('## Done psrecord cmd... ##')

        cpu_values = []
        ram_values = []
        with open(psrecord_output_file) as file:
            for line in file:
                search_result = findall(R'[0-9]+\.[0-9]+', line)
                if len(search_result) == 4:
                    (_, cpu, ram, _) = search_result
                    cpu_values.append(float(cpu))
                    ram_values.append(float(ram))

        max_cpu = max(cpu_values)
        max_mem = max(ram_values)
        avg_cpu = mean(cpu_values)
        avg_mem = mean(ram_values)

        collected_metrics[test_info.name] = {
            "Performance": {"max_cpu": max_cpu, "max_mem": max_mem, "avg_cpu": avg_cpu, "avg_mem": avg_mem}}

        print(f'{test_info.name} measuring DONE ###############################################')


# Runs all tests and collect metrics from the GTest output
def run_all_tests(test_exec, test_infos, collected_metrics):
    all_test_names = ''
    for test_info in test_infos:
        if not all_test_names:
            all_test_names = test_info.name
        else:
            all_test_names += f':{test_info.name}'

    output_file = './test_detail_perf.json'
    test_run_command = get_test_run_command(test_exec, all_test_names, 1, 'json', output_file)
    test_process = subprocess.run(test_run_command)
    if test_process.returncode != 0:
        print(f'Tests failed! Return code {test_process.returncode}')
        exit(test_process.returncode)

    with open(output_file) as file:
        json_output = json.load(file)

    for test_info in test_infos:
        (suite, _, test) = test_info.name.partition('.')
        for metrics_info in test_info.metric_infos:
            if metrics_info.name not in collected_metrics[test_info.name]:
                collected_metrics[test_info.name][metrics_info.name] = {}
            for metric in metrics_info.metrics:
                value = jq.first(
                    f'.testsuites[] | select(.name = "{suite}").testsuite[] | select(.name == "{test}").{metric}',
                    json_output)

                if not value:
                    continue
                if metric.find('bytes') != -1:
                    value = '{:.3f}'.format(int(value) / 1024.0 / 1024.0)
                collected_metrics[test_info.name][metrics_info.name][fix_bytes(metric)] = value

    shutil.move(output_file, WORK_DIR + f'/{output_file}')


# Tries to download previous version of the CSV file with values, if fail creates new file. Then appends all values
def append_or_create_csv_files(artifactory_url, test_infos, collected_metrics):
    built_files = []

    for test_info in test_infos:
        (_, _, test_name) = test_info.name.partition('.')

        for (metrics_info_name, metrics) in test_info.metric_infos:
            # Hardcoded test_name due to problem with slash in name.
            # Slash in filenames are not allowed on Linux. Replace / to _
            test_name='ReadNPartitionsFromVersionedLayer_15m_test'
            output_file_name = f'performance_results_{test_name}_{metrics_info_name}.csv'
            print ({output_file_name})
            # Requesting file from the artifactory
            response = requests.get(artifactory_url + f'{output_file_name}', stream=True)
            print ({response})
            if response.status_code != 200:
                # There is no such file, lets create one
                with open(output_file_name, 'w') as output_file:
                    # First field is always 'version'
                    output_file.write('version')
                    # And then all metrics
                    for field in metrics:
                        output_file.write(f',{fix_bytes(field)}')
                    output_file.write('\n')
            else:
                # There is previous version so just save it
                with open(output_file_name, 'wb') as output_file:
                    shutil.copyfileobj(response.raw, output_file)

            # Writing new values
            with open(output_file_name, 'a') as output_file:
                output_file.write('\n')
                # Version is a job ID
                output_file.write(os.environ['CI_JOB_ID'])

                for field in metrics:
                    value = collected_metrics[test_info.name][metrics_info_name][fix_bytes(field)]
                    if value:
                        output_file.write(f',{value}')
                    else:
                        print(f'Value {field} is absent for {test_info.name}')
                        output_file.write(',0')

            built_files.append(output_file_name)

    return built_files


# Uploads all updated files to the Artifactory
def upload_to_artifactory(artifactory_url, files):
    if DISABLE_UPLOAD_TO_ARTIFACTORY:
        return

    username = os.environ['ARTIF_BOT']
    password = os.environ['ARTIF_BOT_PWD']

    for file in files:
        print(f'Uploading {file} to the Artifactory: {artifactory_url}')
        shell(f'curl -u {username}:{password} -X PUT "{artifactory_url + file}" -T {file}')
        shutil.move(file, WORK_DIR + f'/{file}')


def main():

    artifactory_host = os.environ['ARTIFACTORY_HOST']
    parser = argparse.ArgumentParser()
    parser.add_argument('-j', '--json', help='Path to the JSON file with tests description',
                        default='./scripts/linux/weekly/performance_tests.json')
    parser.add_argument('-html', '--html', help='Path to the html file with UI dashboard',
                        default='./scripts/linux/weekly/reports/index.html')
    parser.add_argument('-t', '--test_exec', help='Path to test executable',
                        default='./build/tests/performance/olp-cpp-sdk-performance-tests')
    parser.add_argument('-a', '--artifactory', help='URL to Artifcatory as upload target',
                        default=f'https://{artifactory_host}/artifactory/edge-sdks/sdk-for-cpp/test-data/')
    args = parser.parse_args()

    # Comment it always before commit
    # setup_for_local_usage()

    # Parsers for json and html
    update_system_info(args.html)
    update_system_info(args.json)

    test_infos = load_tests(args.json)
    all_metrics = {}
    calculate_cpu_and_memory(args.test_exec, test_infos, all_metrics)

    # Needed for more complicated measurements in future
    #run_all_tests(args.test_exec, test_infos, all_metrics)

    pprint.pprint(all_metrics)

    all_files = append_or_create_csv_files(args.artifactory, test_infos, all_metrics)
    upload_to_artifactory(args.artifactory, all_files)


if __name__ == '__main__':
    main()
