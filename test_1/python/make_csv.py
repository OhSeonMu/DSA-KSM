# -*- coding: utf-8 -*- 
import csv
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("ksm_type", help="KSM Type")
parser.add_argument("workload_type", help="Workload Type")
parser.add_argument("workload_size", help="Workload Size")
args = parser.parse_args()

input_file = "/home/osm/test_1/output/" + args.ksm_type + "_" + args.workload_type + "_" + args.workload_size + ".dat"
output_file = "/home/osm/test_1/output/" + args.ksm_type + "_" + args.workload_type + "_" + args.workload_size + ".csv"

#input_file = "/home/osm/script/output/default_ksm.dat"
#input_file = "/home/osm/script/output/dsa_ksm.dat"
#input_file = "/home/osm/script/output/no_ksm.dat"
#output_file = "/home/osm/script/output/default_ksm.csv"
#output_file = "/home/osm/script/output/dsa_ksm.csv"
#output_file = "/home/osm/script/output/no_ksm.csv"

with open(output_file, 'w') as csv_file:
  csv_writer = csv.writer(csv_file)
  
  with open(input_file, 'r') as input_file:
    for line in input_file:
      fields = line.split()
      csv_writer.writerow(fields)

