# import struct
# import sys 

# def count_lines(filename):
#     with open(filename, 'r') as f:
#         return sum(1 for _ in f)

# def split_into_binary_files(input_file, n):
#     total_lines = count_lines(input_file)
#     lines_per_file = total_lines // n
#     remainder = total_lines % lines_per_file

#     with open(input_file, 'r') as f:
#         for i in range(n):
#             if i == n - 1 and i != 0: 
#                 lines_this_file = remainder
#             else:        
#                 lines_this_file = lines_per_file 
#             output_file = f'output_part{i+1}.bin'
#             with open(output_file, 'wb') as out:
#                 for _ in range(lines_this_file):
#                     line = f.readline()
#                     parts = line.strip().split()
#                     if len(parts) == 3 and parts[0] == 'p':
#                         int1, int2 = int(parts[1]), int(parts[2])
#                         packed_data = struct.pack('ii', int1, int2)
#                         out.write(packed_data)
#             with open(f"client_input{i}.txt", 'w') as o:
#                 o.write(f"l {output_file}") 
#                 if (i == n -1):
#                     o.write("\nq")
#                 else: 
#                     o.write("\ncq")
# # Usage
# split = sys.argv[1]
# split_into_binary_files('workloads/workload_3.txt', int(split))

import sys 

split = int(sys.argv[1])
for i in range(split):
    with open(f"client_input{i}.txt", 'w') as o:
                    o.write(f"l workloads/load_{split}_{i}.dat") 
                    if (i == split -1):
                        o.write("\nq")
                    else: 
                        o.write("\ncq")