import argparse, sys, re
parser = argparse.ArgumentParser()

# Hardware Revision Pattern (Semantic Versioned) e.g. 0.1.0
hw_pattern = re.compile("^[\d+]*[.][\d]*[.][\d]*$")

# Serial Number Pattern
# A##-YYWW-####
# A(amp revision)-(year last two)(week number)-(unique identifier)
sn_pattern = re.compile("^A[0-9a-zA-Z]{2}-[0-9]{4}-[0-9a-zA-Z]{4}$")

parser.add_argument('--hw', help='Hardware Revision')
parser.add_argument('--sn', help='Serial Number')
parser.add_argument('--output', '-o', help="Output File")

args = parser.parse_args()

def get_hw_bytes():
  split_hw = args.hw.split(".")
  major = int(split_hw[0])
  minor = int(split_hw[1])
  patch = int(split_hw[2])
  return [major, minor, patch]

def get_sn_bytes():
  split_sn = args.sn.split("-")
  model = int("0x{0}".format(split_sn[0][1:]), 16)
  year = int(split_sn[1][0:2])
  week = int(split_sn[1][2:])
  unique_one = int("0x{0}".format(split_sn[2][0:2]), 16)
  unique_two = int("0x{0}".format(split_sn[2][2:]), 16)
  return [model, year, week, unique_one, unique_two]

if not args.hw or not args.sn or not bool(hw_pattern.match(args.hw)) or not bool(sn_pattern.match(args.sn)):
  print("valid hw and sn required")

if (not args.output):
  filename = "device_id.bin"
else:
  filename = args.output

data = []
data.extend(get_hw_bytes())
data.extend(get_sn_bytes())

output = open(filename, "wb")
output.write(bytearray(data))