//
// Created by consti10 on 04.11.20.
//

#ifndef LIVEVIDEO10MS_H264_H
#define LIVEVIDEO10MS_H264_H

#include <h264_stream.h>
#include "NALUnitType.hpp"

// namespaces for H264 H265 helper
// A H265 NALU is kind of similar to a H264 NALU in that it has the same [0,0,0,1] prefix

namespace H26X{
    // The rbsp buffer starts after the 0,0,0,1 header
    // This function *reverts' the fact that a NAL unit mustn't contain the [0,0,0,1] pattern
    static std::vector<uint8_t> nalu_to_rbsp_buff(const std::vector<uint8_t>& naluData){
        int nal_size = naluData.size()-4;
        const uint8_t* nal_data=&naluData[4];
        int rbsp_size=nal_size;
        std::vector<uint8_t> rbsp_buf;
        rbsp_buf.resize(rbsp_size);
        int rc = nal_to_rbsp(nal_data, &nal_size, rbsp_buf.data(), &rbsp_size);
        assert(rc>0);
        assert(rbsp_buf.size()==rbsp_size);
        return rbsp_buf;
    }
    static std::vector<uint8_t> nalu_to_rbsp_buff(const uint8_t* nalu_data,std::size_t nalu_data_size){
        return nalu_to_rbsp_buff(std::vector<uint8_t>(nalu_data,nalu_data+nalu_data_size));
    }
}

namespace H264{
    // reverse order due to architecture
    typedef struct nal_unit_header{
        uint8_t nal_unit_type:5;
        uint8_t nal_ref_idc:2;
        uint8_t forbidden_zero_bit:1;
        std::string asString()const{
            std::stringstream ss;
            ss<<"nal_unit_type:"<<(int)nal_unit_type<<" nal_ref_idc:"<<(int)nal_ref_idc<<" forbidden_zero_bit:"<<(int)forbidden_zero_bit;
            return ss.str();
        }
    }__attribute__ ((packed)) nal_unit_header_t;
    static_assert(sizeof(nal_unit_header_t)==1);
    typedef struct slice_header{
        uint8_t frame_num:2;
        uint8_t pic_parameter_set_id:2;
        uint8_t slice_type:2;
        uint8_t first_mb_in_slice:2 ;
        std::string asString()const{
            std::stringstream ss;
            ss<<"first_mb_in_slice:"<<(int)first_mb_in_slice<<" slice_type:"<<(int)slice_type<<" pic_parameter_set_id:"<<(int)pic_parameter_set_id<<" frame_num:"<<(int)frame_num;
            return ss.str();
        }
    }__attribute__ ((packed)) slice_header_t;
    static_assert(sizeof(slice_header_t)==1);

    static std::string spsAsString(const sps_t* sps){
        std::stringstream ss;
        ss<<"[";
        ss<<"profile_idc="<<sps->profile_idc<<",";
        ss<<"constraint_set0_flag="<<sps->constraint_set0_flag<<",";
        ss<<"constraint_set1_flag="<<sps->constraint_set1_flag<<",";
        ss<<"constraint_set2_flag="<<sps->constraint_set2_flag<<",";
        ss<<"constraint_set3_flag="<<sps->constraint_set3_flag<<",";
        ss<<"constraint_set4_flag="<<sps->constraint_set4_flag<<",";
        ss<<"constraint_set5_flag="<<sps->constraint_set5_flag<<",";
        ss<<"reserved_zero_2bits="<<sps->reserved_zero_2bits<<",";
        ss<<"level_idc="<<sps->level_idc<<",";
        ss<<"seq_parameter_set_id="<<sps->seq_parameter_set_id<<",";
        ss<<"chroma_format_idc="<<sps->chroma_format_idc<<",";
        ss<<"residual_colour_transform_flag="<<sps->residual_colour_transform_flag<<",";
        ss<<"bit_depth_luma_minus8="<<sps->bit_depth_luma_minus8<<",";
        ss<<"bit_depth_chroma_minus8="<<sps->bit_depth_chroma_minus8<<",";
        ss<<"qpprime_y_zero_transform_bypass_flag="<<sps->qpprime_y_zero_transform_bypass_flag<<",";
        ss<<"seq_scaling_matrix_present_flag="<<sps->seq_scaling_matrix_present_flag<<",";
        ss<<"log2_max_frame_num_minus4="<<sps->log2_max_frame_num_minus4<<",";
        ss<<"pic_order_cnt_type="<<sps->pic_order_cnt_type<<",";
        ss<<"log2_max_pic_order_cnt_lsb_minus4="<<sps->log2_max_pic_order_cnt_lsb_minus4<<",";
        ss<<"delta_pic_order_always_zero_flag="<<sps->delta_pic_order_always_zero_flag<<",";
        //ss<<"="<<sps-><<",";
        ss<<"]";
        return ss.str();
    }
    // Parse raw NALU data into an sps struct (using the h264bitstream library)
    class SPS{
    public:
        nal_unit_header_t nal_header;
        sps_t parsed;
    public:
        // data buffer= NALU data with prefix
        SPS(const uint8_t* nalu_data,size_t data_len){
            auto rbsp_buf= H26X::nalu_to_rbsp_buff(nalu_data, data_len);
            bs_t* b = bs_new(rbsp_buf.data(), rbsp_buf.size());
            nal_header.forbidden_zero_bit=bs_read_u1(b);
            nal_header.nal_ref_idc = bs_read_u(b, 2);
            nal_header.nal_unit_type = bs_read_u(b, 5);
            assert(nal_header.forbidden_zero_bit==0);
            assert(nal_header.nal_unit_type==NAL_UNIT_TYPE_SPS);
            read_seq_parameter_set_rbsp(&parsed, b);
            read_rbsp_trailing_bits(b);
            bs_free(b);
        }
        std::array<int,2> getWidthHeightPx()const{
            int Width = ((parsed.pic_width_in_mbs_minus1 +1)*16) -parsed.frame_crop_right_offset *2 -parsed.frame_crop_left_offset *2;
            int Height = ((2 -parsed.frame_mbs_only_flag)* (parsed.pic_height_in_map_units_minus1 +1) * 16) - (parsed.frame_crop_bottom_offset* 2) - (parsed.frame_crop_top_offset* 2);
            return {Width,Height};
        }
        std::string asString()const{
            return spsAsString(&parsed);
        }
    };
}

namespace H265{
    typedef struct nal_unit_header{
        uint8_t forbidden_zero_bit:1;
        uint8_t nal_unit_type:6;
        uint8_t nuh_layer_id:6;
        uint8_t nuh_temporal_id_plus1:3;
    }__attribute__ ((packed)) nal_unit_header_t;
    static_assert(sizeof(nal_unit_header_t)==2);

    typedef struct h265_sps{
        uint8_t sps_video_parameter_set_id;
        uint8_t sps_max_sub_layers_minus1;
        uint8_t sps_temporal_id_nesting_flag;
        uint8_t sps_seq_parameter_set_id;
        uint8_t chroma_format_idc;
        uint8_t pic_width_in_luma_samples;
        uint8_t pic_height_in_luma_samples;
        uint8_t conformance_window_flag;
        uint8_t bit_depth_luma_minus8;
        uint8_t bit_depth_chroma_minus8;
        uint8_t log2_max_pic_order_cnt_lsb_minus4;
        uint8_t sps_sub_layer_ordering_info_present_flag;
        uint8_t log2_min_luma_coding_block_size_minus3;
        uint8_t log2_diff_max_min_luma_coding_block_size;
        uint8_t log2_min_luma_transform_block_size_minus2;
        uint8_t log2_diff_max_min_luma_transform_block_size;
        uint8_t max_transform_hierarchy_depth_inter;
        uint8_t max_transform_hierarchy_depth_intra;
        uint8_t scaling_list_enabled_flag;
        uint8_t amp_enabled_flag;
        uint8_t sample_adaptive_offset_enabled_flag;
        uint8_t pcm_enabled_flag;
        uint8_t num_short_term_ref_pic_sets;
    }h265_sps_t;
    static void read_h265_seq_parameter_set_rbsp(h265_sps_t& sps,bs_t* b){
        memset(&sps, 0, sizeof(sps_t));
        sps.sps_video_parameter_set_id=bs_read_u4(b);
        sps.sps_max_sub_layers_minus1 = bs_read_u1(b);
        sps.sps_temporal_id_nesting_flag = bs_read_u1(b);
        sps.sps_seq_parameter_set_id = bs_read_u1(b);
        sps.chroma_format_idc = bs_read_u1(b);
        sps.pic_width_in_luma_samples = bs_read_u1(b);
        sps.pic_height_in_luma_samples = bs_read_u1(b);
        sps.conformance_window_flag = bs_read_u1(b);
        sps.bit_depth_luma_minus8 = bs_read_u1(b);
        sps.bit_depth_chroma_minus8 = bs_read_u1(b);
        sps.log2_max_pic_order_cnt_lsb_minus4 = bs_read_u1(b);
        sps.sps_sub_layer_ordering_info_present_flag = bs_read_u1(b);
    }

    class SPS{
    public:
        nal_unit_header_t nal_header;
        h265_sps_t parsed;
    public:
        // data buffer= NALU data with prefix
        SPS(const uint8_t* nalu_data,size_t data_len){
            auto rbsp_buf= H26X::nalu_to_rbsp_buff(nalu_data, data_len);
            bs_t* b = bs_new(rbsp_buf.data(), rbsp_buf.size());
            nal_header.forbidden_zero_bit=bs_read_u1(b);
            nal_header.nal_unit_type = bs_read_u(b, 6);
            nal_header.nuh_layer_id=bs_read_u(b,6);
            nal_header.nuh_temporal_id_plus1=bs_read_u(b,3);
            assert(nal_header.forbidden_zero_bit==0);
            assert(nal_header.nal_unit_type==NALUnitType::H265::NAL_UNIT_SPS);
            //read_seq_parameter_set_rbsp(&parsed, b);
            read_rbsp_trailing_bits(b);
            bs_free(b);
        }
    };
}

#endif //LIVEVIDEO10MS_H264_H
