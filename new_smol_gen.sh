#python configs/generate/mesh.py --output_file configs/topology/jetson_mesh_4.json --mesh_row 4 --mesh_col 4 --noc_bandwidth 48 --dram_bandwidth 24

#python configs/generate/mesh_arch_0.py --output_file configs/arch/jetson_mesh_4_0.ini --mesh_row 4 --mesh_col 4 --buffer_size 2048 --sa_frequency_ghz 1.5 --sa_row 16 --sa_col 16 --sa_num_array 4
python configs/generate/gemm.py --output_file configs/phase/smol_v_qkv.json --data_type fp16 --m 3072 --n 2304 --k 768  --m_tile_size 384 --n_tile_size 384 --k_tile_size 384
python configs/generate/attention.py --output_file configs/phase/smol_v_attn.json --data_type fp16 --batch 3 --seq_len 1024 --kv_size 1024 --head_dim 64 --head_num 12 --seq_tile_size 256 --head_tile_size 6 --kv_tile_size 256
python configs/generate/gemm.py --output_file configs/phase/smol_v_out.json --data_type fp16 --m 3072 --n 768 --k 768  --m_tile_size 256 --n_tile_size 384 --k_tile_size 384
python configs/generate/gemm.py --output_file configs/phase/smol_v_up.json --data_type fp16 --m 3072 --n 3072 --k 768  --m_tile_size 384 --n_tile_size 384 --k_tile_size 384
python configs/generate/gemm.py --output_file configs/phase/smol_v_down.json --data_type fp16 --m 3072 --n 768 --k 3072  --m_tile_size 384 --n_tile_size 384 --k_tile_size 384

python configs/generate/gemm.py --output_file configs/phase/smol_l_qkv.json --data_type fp16 --m 240 --n 1600 --k 960  --m_tile_size 120 --n_tile_size 200 --k_tile_size 160
python configs/generate/attention.py --output_file configs/phase/smol_l_attn.json --data_type fp16 --batch 1 --seq_len 720 --kv_size 240 --head_dim 64 --head_num 5 --seq_tile_size 180 --head_tile_size 1 --kv_tile_size 80
python configs/generate/gemm.py --output_file configs/phase/smol_l_out.json --data_type fp16 --m 240 --n 960 --k 960  --m_tile_size 120 --n_tile_size 120 --k_tile_size 160
python configs/generate/gemm.py --output_file configs/phase/smol_l_up.json --data_type fp16 --m 240 --n 5120 --k 960  --m_tile_size 240 --n_tile_size 160 --k_tile_size 240 --gate
python configs/generate/gemm.py --output_file configs/phase/smol_l_down.json --data_type fp16 --m 240 --n 960 --k 2560  --m_tile_size 120 --n_tile_size 240 --k_tile_size 160

python configs/generate/gemm.py --output_file configs/phase/smol_a_qkv.json --data_type fp16 --m 50 --n 1600 --k 720  --m_tile_size 50 --n_tile_size 400 --k_tile_size 360
python configs/generate/attention.py --output_file configs/phase/smol_a_self.json --data_type fp16 --batch 1 --seq_len 150 --kv_size 50 --head_dim 64 --head_num 5 --seq_tile_size 50 --head_tile_size 1 --kv_tile_size 50
python configs/generate/gemm.py --output_file configs/phase/smol_a_out.json --data_type fp16 --m 50 --n 720 --k 960  --m_tile_size 50 --n_tile_size 360 --k_tile_size 240
python configs/generate/gemm.py --output_file configs/phase/smol_a_q.json --data_type fp16 --m 50 --n 960 --k 720  --m_tile_size 50 --n_tile_size 240 --k_tile_size 360
python configs/generate/attention.py --output_file configs/phase/smol_a_cross.json --data_type fp16 --batch 1 --seq_len 150 --kv_size 240 --head_dim 64 --head_num 5 --seq_tile_size 150 --head_tile_size 1 --kv_tile_size 80
python configs/generate/gemm.py --output_file configs/phase/smol_a_up.json --data_type fp16 --m 50 --n 4096 --k 720  --m_tile_size 50 --n_tile_size 512 --k_tile_size 720 --gate
python configs/generate/gemm.py --output_file configs/phase/smol_a_down.json --data_type fp16 --m 50 --n 720 --k 2048  --m_tile_size 50 --n_tile_size 360 --k_tile_size 512
