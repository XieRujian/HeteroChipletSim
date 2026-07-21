python configs/generate/mesh_arch_4_loc1.py --output_file configs/arch/mesh_4_4_loc1.ini --mesh_row 4 --mesh_col 4 --buffer_size 128
python configs/generate/mesh_arch_4_loc2.py --output_file configs/arch/mesh_4_4_loc2.ini --mesh_row 4 --mesh_col 4 --buffer_size 128
python configs/generate/mesh_arch_4_loc3.py --output_file configs/arch/mesh_4_4_loc3.ini --mesh_row 4 --mesh_col 4 --buffer_size 128

python configs/generate/mesh.py --output_file configs/topology/oc_mesh_4_2x.json --mesh_row 4 --mesh_col 4 --noc_bandwidth 32 --dram_bandwidth 16
