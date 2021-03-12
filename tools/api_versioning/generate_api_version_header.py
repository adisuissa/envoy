#!/usr/bin/python
"""Parses a file containing the API version (X.Y.Z format), and outputs (to
stdout) a C++ header file with the ApiVersion value.
"""
import pathlib
import string
import sys
import utils
from utils import ApiVersion

FILE_TEMPLATE = string.Template("""#pragma once
#include "common/version/api_version_struct.h"
namespace Envoy {
constexpr ApiVersion api_version = {$major, $minor, $patch};
constexpr ApiVersion oldest_api_version = {$oldest_major, $oldest_minor, $oldest_patch};
} // namespace Envoy""")


def GenerateHeaderFile(input_path):
  """Generates a c++ header file containing the api_version variable with the
  correct value.
  Args:
    input_path: the file containing the API version (API_VERSION).
  Returns:
    the header file contents.
  """
  version = utils.GetApiVersion(input_path)
  oldest_version = utils.ComputeOldestApiVersion(version)

  header_file_contents = FILE_TEMPLATE.substitute({
      'major': version.major,
      'minor': version.minor,
      'patch': version.patch,
      'oldest_major': oldest_version.major,
      'oldest_minor': oldest_version.minor,
      'oldest_patch': oldest_version.patch
  })
  return header_file_contents


if __name__ == '__main__':
  input_path = sys.argv[1]
  output = GenerateHeaderFile(input_path)
  # Print output to stdout
  print(output)
