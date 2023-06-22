#!/usr/bin/env python3
"""
This script parse firmware log
"""
import ctypes
import getopt
import os.path
import re
import sys
from ctypes import Structure, c_uint16, c_uint32

from typing import List, Any

from xml.dom.minidom import parse
import xml.dom.minidom


class Dword1(Structure):
    _fields_ = [('magic_number', c_uint32, 8),
                ('severity', c_uint32, 5),
                ('thread_id', c_uint32, 3),
                ('field_id', c_uint32, 11),
                ('group_id', c_uint32, 5)]


class Dword1Union(ctypes.Union):
    _fields_ = [("bytes", Dword1),
                ("as_byte", c_uint32)]


class Dword2(Structure):
    _fields_ = [('event_id', c_uint32, 16),
                ('line_number', c_uint32, 12),
                ('sequence', c_uint32, 4)]


class Dword2Union(ctypes.Union):
    _fields_ = [("bytes", Dword2),
                ("as_byte", c_uint32)]


class Dword3(Structure):
    _fields_ = [('data1', c_uint16),
                ('data2', c_uint16)]


class Dword3Union(ctypes.Union):
    _fields_ = [("bytes", Dword3),
                ("as_byte", c_uint32)]


class Dword4(Structure):
    _fields_ = [('data3', c_uint32)]


class Dword4Union(ctypes.Union):
    _fields_ = [("bytes", Dword4),
                ("as_byte", c_uint32)]


class Dword5(Structure):
    _fields_ = [('timestamp', c_uint32)]


class Dword5Union(ctypes.Union):
    _fields_ = [("bytes", Dword5),
                ("as_byte", c_uint32)]


def usage() -> None:
    """
    This function print help menu on the screen
    :return: None
    """
    script_name = os.path.basename(sys.argv[0])

    print('This script parse firmware log.                                ')
    print('In the output_customisation dictionary you can customize output')
    print('Syntax: ' + script_name + ' [-f] <firmware_log> [-x] <xml_file>')
    print('OR                                                             ')
    print('Syntax: output | ' + script_name + ' [-x] <xml_file>           ')
    print('                       -h, --help         prints help info     ')
    print('                       -f, --log-file     firmware log file    ')
    print('                       -x, --xml-events   xml file             ')
    exit(1)


def read_log_file(file_link: str) -> str:
    """
    Read all data from log file
    :param file_link: link to the file with a log
    :type file_link: str
    :return: all read log
    :rtype: str
    """
    if type(file_link) is not str or not os.path.exists(file_link) or not os.path.isfile(file_link):
        print(f'Error: file {file_link} not found')
        exit(1)

    with open(file_link, 'r') as file:
        data = file.read()

    return data


def read_xml_file(file_link: str):
    """
    This function read xml file with parsing rules
    :param file_link: xml link
    :type file_link: str
    :return: object with tree xml data
    """
    # Open XML document using minidom parser
    if xml_file_link and type(xml_file_link) is str and os.path.exists(xml_file_link) and os.path.isfile(xml_file_link):
        if xml_file_link.endswith('.xml'):
            dom_tree = xml.dom.minidom.parse(xml_file_link)
            data = dom_tree.documentElement
            return data
        else:
            print(f'Error: file {file_link} has wrong format')
            exit(1)
    else:
        print(f'Error: file {file_link} not found')
        exit(1)


def read_pipe_input():
    """
    This function read input from pipe
    :return: input from pipe
    :rtype: str
    """
    lines = ''
    for log_line in sys.stdin:
        lines += log_line

    return lines


def remove_lines_containing_only_zeros(matrix: List[list]):
    """
    This function remove line with zeros from a matrix
    :param matrix: List that contains List of strings
    :type matrix: List[list] str
    :return: new List that contains List of strings
    :rtype: List[list] str
    """
    new_matrix = []

    for line in matrix:
        if not all(num == '0' for num in line) and len(line) > 0:
            new_matrix.append(line)

    return new_matrix


def remove_first_bytes(log: list, number_bytes: int) -> list:
    """
    This function remove first N given nodes from a list.
    :param log: list with strings
    :type log: list
    :param number_bytes: numer of first nodes that needs remove
    :type number_bytes: int
    :return: edited list
    :rtype: list
    """
    if log and type(log) is list and type(number_bytes) is int and 0 < number_bytes < len(log):
        for i in range(number_bytes):
            log.pop(0)

    return log


def split_log(log: list, length_of_list: int = 20) -> List[List[Any]]:
    """
    This function splits one list to list with length length_of_list size.
    :param log: list of data
    :type log: list
    :param length_of_list: size of small list
    :type length_of_list: int
    :return: list of list
    :rtype: list
    """
    bytes_in_log_line = 20

    if log and type(log) is list and type(length_of_list) is int:
        return [log[i:i + bytes_in_log_line] for i in range(0, len(log), length_of_list)]


def print_log_headers() -> None:
    """
    Print headers to a console
    :return: None
    """
    print_format_log_line('Sequence', 'File name', 'Group id', 'Thread name', 'Severity',
                          'Line', 'Timestamp', '\u0394 timestamp', 'Description')


def get_file_name_from_xml(_id: str) -> str:
    """
    This function get file name from xml data by file id
    :param _id: file id number
    :type _id: str
    :return: file name
    :rtype: str
    """
    global xml_data

    if type(_id) is str:
        files = xml_data.getElementsByTagName('File')

        for f in files:
            if f.hasAttribute('id') and f.hasAttribute('Name'):
                file_id_xml = f.getAttribute('id')
                if file_id_xml == _id:
                    return f.getAttribute('Name')

    return 'File not found'


def get_thread_name_from_xml(thread_id: int) -> str:
    """
    Scan xml for thread name according given thread id
    :param tread_id: thread id
    :type tread_id: int
    :return: thread name
    :rtype str
    """
    global xml_data

    if type(thread_id) is int:
        threads = xml_data.getElementsByTagName('Thread')

        for t in threads:
            if t.hasAttribute('id') and t.hasAttribute('Name'):
                thread_id_xml = t.getAttribute('id')
                if thread_id_xml == str(thread_id):
                    return t.getAttribute('Name')

    return 'Thread not found'


def get_format_string_from_xml(_id: str) -> str:
    """
    This function get format string from xml data by event id
    :param _id: event id
    :type _id: str
    :return: format string
    :rtype: str
    """
    global xml_data

    if type(_id) is str:
        events = xml_data.getElementsByTagName('Event')

        for event in events:
            if event.hasAttribute('id') and event.hasAttribute('format'):
                id_xml = event.getAttribute('id')
                if id_xml == _id:
                    return event.getAttribute('format')

    return 'Event not found'


def get_number_of_arguments_from_xml(_id: str) -> int:
    """
    This function get amount of arguments in format by event id from xml file
    :param: event_id: event id
    :type: event_id: str
    :return: amount of parameters
    :rtype: int
    """
    global xml_data

    if type(_id) is str:
        events = xml_data.getElementsByTagName('Event')

        for event in events:
            if event.hasAttribute('id') and event.hasAttribute('numberOfArguments'):
                id_xml = event.getAttribute('id')
                if id_xml == _id:
                    return int(event.getAttribute('numberOfArguments'))

    return 0


def get_double_word(bytes_: list) -> int:
    """
    This function return double word
    :param bytes_: bytes in row log
    :type bytes_: list
    :return: double word
    :rtype: int
    """
    global byte_pointer
    size_of_byte = 8
    bytes_in_dword = 4

    dword = 0

    for i in range(bytes_in_dword):
        byte = int(bytes_[byte_pointer])
        byte <<= i * size_of_byte
        dword |= byte

        byte_pointer += 1

    return dword


def get_description_string(format_str: str, number_args: int, var_1, var_2, var_3) -> str:
    """
    This function build description string
    :param format_str: format string
    :type format_str: str
    :param number_args: number of arguments that string expecting
    :type number_args: int
    :param var_1:
    :type var_1:
    :param var_2:
    :type var_2:
    :param var_3:
    :type var_3:
    :return: final description string
    :rtype: int
    """
    if number_args == 0:
        string = format_str
        return string
    elif number_args == 1:
        string = format_str.format(var_1)
        return string
    elif number_args == 2:
        string = format_str.format(var_1, var_2)
        return string
    elif number_args == 3:
        string = format_str.format(var_1, var_2, var_3)
        return string
    else:
        return format_str + " (Wrong number of arguments read from the log line!)"


def print_format_log_line(seq_id, f_name, g_id, thread_name_, severity_, line_num,
                          timestamp_, delta_timestamp_, description) -> None:
    """
    Prints string of parsed log line according format
    :param seq_id: sequence id
    :param f_name: file name
    :param g_id: group id
    :param thread_name_: thread name
    :param severity_: severity
    :param line_num: line number
    :param timestamp_: timestamp
    :param delta_timestamp_: delta timestamp
    :param description: error description
    """
    global output_customisation

    if output_customisation['print_sequence_id']:
        print('{:<10}'.format(seq_id), end='')

    if output_customisation['print_file_name']:
        print('{:<30}'.format(f_name), end='')

    if output_customisation['print_group_id']:
        print('{:<10}'.format(g_id), end='')

    if output_customisation['print_thread_name']:
        print('{:<13}'.format(thread_name_), end='')

    if output_customisation['print_severity']:
        print('{:<10}'.format(severity_), end='')

    if output_customisation['print_line_num']:
        print('{:<6}'.format(line_num), end='')

    if output_customisation['print_timestamp']:
        print('{:<15}'.format(timestamp_), end='')

    if output_customisation['print_delta_timestamp']:
        if type(delta_timestamp_) == float:
            print('{:<13}'.format(str(delta_timestamp_)[0: 7: 1]), end='')
        else:
            print('{:<13}'.format(str(delta_timestamp_)), end='')  # print header

    if output_customisation['print_description']:
        print('{:<150}'.format(description), end='')

    print('\n')


def calculate_delta_timestamp(timestamp, last_timestamp) -> int:
    """
    Calculates delta timestamp between previous and current timestamp
    :param timestamp:
    :type timestamp:
    :param last_timestamp:
    :type last_timestamp:
    :return: delta timestamp
    :rtype: int
    """
    timestamp_factor = 0.00001

    if not last_timestamp:
        return 0
    else:
        return (timestamp - last_timestamp) * timestamp_factor


if __name__ == '__main__':

    output_customisation = {'print_sequence_id': True,
                            'print_file_name': True,
                            'print_group_id': True,
                            'print_thread_name': True,
                            'print_severity': True,
                            'print_line_num': True,
                            'print_timestamp': True,
                            'print_delta_timestamp': True,
                            'print_description': True}

    opts = []
    args = ""

    # parse command-line:
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hf:x:", longopts=['help', 'log-file', 'xml-events'])
    except getopt.GetoptError as err:
        print("Error in get opt")
        usage()

    log_file_link = ''
    xml_file_link = ''

    for opt, arg in opts:

        if opt in ('-h', '--help'):
            usage()
        elif opt in ('-f', '--log-file'):
            log_file_link = arg
        elif opt in ('-x', '--xml-events'):
            xml_file_link = arg

    xml_data = read_xml_file(xml_file_link)
    if log_file_link == '':
        logs_str = read_pipe_input()
    else:
        logs_str = read_log_file(log_file_link)

    # remove all except numbers and ','
    logs_str = re.sub("[^0-9,]", "", logs_str)

    logs_list = logs_str.split(',')

    logs_list = remove_first_bytes(logs_list, 4)
    logs_matrix = split_log(logs_list)
    logs_matrix = remove_lines_containing_only_zeros(logs_matrix)

    last_timestamp = 0

    print_log_headers()

    for row_bytes in logs_matrix:
        byte_pointer = 0

        dword1 = Dword1Union()
        dword1.as_byte = get_double_word(row_bytes)

        dword2 = Dword2Union()
        dword2.as_byte = get_double_word(row_bytes)

        dword3 = Dword3Union()
        dword3.as_byte = get_double_word(row_bytes)

        dword4 = Dword4Union()
        dword4.as_byte = get_double_word(row_bytes)

        dword5 = Dword5Union()
        dword5.as_byte = get_double_word(row_bytes)

        # double word 1 scope
        magic_number = dword1.bytes.magic_number
        severity = dword1.bytes.severity
        thread_id = dword1.bytes.thread_id
        file_id = dword1.bytes.field_id
        group_id = dword1.bytes.group_id

        # double word 2 scope
        event_id = dword2.bytes.event_id
        line_number = dword2.bytes.line_number
        sequence = dword2.bytes.sequence

        # double word 3 scope
        data_1 = dword3.bytes.data1
        data_2 = dword3.bytes.data2

        # double word 4 scope
        data_3 = dword4.bytes.data3

        # double word 5 scope
        timestamp = dword5.bytes.timestamp

        file_name = get_file_name_from_xml(str(file_id))
        thread_name = get_thread_name_from_xml(thread_id)
        delta_timestamp = calculate_delta_timestamp(timestamp, last_timestamp)
        last_timestamp = timestamp

        number_arguments = get_number_of_arguments_from_xml(str(event_id))
        format_string = get_format_string_from_xml(str(event_id))
        description_string = get_description_string(format_string, number_arguments, data_1, data_2, data_3)

        print_format_log_line(sequence, file_name, group_id, thread_name, severity, line_number, timestamp, delta_timestamp,
                              description_string)
