#!/usr/bin/python
# ^-^ coding: utf-8 ^-^

import os, sys, json, traceback

META_DIR = "/noah/tmp/matrix_meta"
#META_DIR = "/home/users/wangfenglei/abc/matrix_meta"

def _convert_instance_state(state_str):
    state_trans_table = {
        'NULL'      :  0,
        'RUNNING'   : 30,
        'DEPLOYING' : 10,
        'ERROR'     : 40,
        'FATAL'     : 40,
        'REMOVING'  : 50,
    }
    current, target = state_str.split('_')
    if target == 'NULL':
        state = (state_trans_table[current], 0)
    else:
        state = (state_trans_table[target], 1)

    return state

def _convert_error_code(error_code):
    error_code_trans_table = {
        'SUCCESS'         :  0,
        'INSTALL_TIMEOUT' : 10,
        'INSTALL_FAIL'    : 20,
        'HEALCHECK_FAIL'  : 40,
        'REMOVE_TIMEOUT'  : 50,
        'REMOVE_FAIL'     : 60,
    }

    return error_code_trans_table[error_code]

def back_up():
    backup_dir = META_DIR + "_backup"
    if os.path.isdir(backup_dir):
        os.system("rm " + META_DIR + "/*")
        os.system("cp " + backup_dir + "/*" + " " + META_DIR)
    else:
        os.system("rm -rf " + backup_dir)
        os.system("cp -r " + META_DIR + " " + backup_dir)


def convert(content):
    old = json.loads(content)
    new = {}

    new['generation'] = old['generation']

    # timestamp and timeo
    if old['install_cmd_timestamp']['tv_sec'] == 0 and \
       old['install_cmd_timestamp']['tv_usec'] == 0:
        new['event_start_timestamp'] = old['remove_cmd_timestamp']['tv_sec']
        new['event_timeout_sec']     = old['remove_timeout_sec']
    else:
        new['event_start_timestamp'] = old['install_cmd_timestamp']['tv_sec']
        new['event_timeout_sec']     = old['deploy_timeout_sec']

    if new['event_start_timestamp'] == 0:
        new['event_timeout_sec'] = 0

    #instance info
    instance_info = {}
    instance_info['service_name'] = old['service_name']
    instance_info['offset']       = old['offset']

    instance_state = _convert_instance_state(old['instance_state'])
    select_meta = 'current_meta' if instance_state[1] == 0 else 'target_meta'
    instance_info['instance_state'] = instance_state[0]

    if instance_state[0] != 10 and instance_state[0] != 50:
        new['event_start_timestamp'] = 0
        new['event_timeout_sec']     = 0

    instance_info['error_code'] = _convert_error_code(old['error_code'])

    #instance meta
    instance_meta = {}
    instance_meta['meta_version']    = old[select_meta]['meta_version']
    instance_meta['package_source']  = old[select_meta]['package_source']
    instance_meta['package_version'] = old[select_meta]['package_version']
    instance_meta['package_type']    = 0
    instance_meta['deploy_dir']      = old[select_meta]['work_dir']

    #instance meta's port
    if old[select_meta]['port'] is None :
        port = None
    else:
        port = {}
        for tmp in old[select_meta]['port'].keys():
            port[old[select_meta]['port'][tmp]] = int(tmp)

    instance_meta['port'] = port
    instance_meta['min_port_include'] = 2000
    instance_meta['max_port_include'] = 10000

    #instance meta's process name
    if old[select_meta]['process_name'] is None :
        process_name = None
    else:
        process_name = {}
        for tmp in old[select_meta]['process_name'].keys():
            process_name[old[select_meta]['process_name'][tmp]] = tmp

    instance_meta['process_name'] = process_name

    #instance meta's others
    instance_meta['tag']                = old[select_meta]['tag']
    instance_meta['resource']           = old[select_meta]['resource']
    instance_meta['deploy_timeout_sec'] = old[select_meta]['deploy_timeout_sec']
    instance_meta['health_check_timeout_sec'] = 10
    instance_meta['group']              = old[select_meta]['group']

    instance_info['instance_meta'] = instance_meta
    new['instance_info'] = instance_info

    return json.dumps(new, indent=4)

def main():
    files = os.listdir(META_DIR)
    for file in files:
        fh = open(META_DIR + "/" + file)
        result = convert(fh.read())
        fh.close()
        fh = open(META_DIR + "/" + file, 'w')
        fh.write(result)
        fh.close()


if __name__ == "__main__":
    back_up()
    main()
