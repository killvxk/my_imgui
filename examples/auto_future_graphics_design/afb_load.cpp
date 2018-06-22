#include "afb_load.h"
#include <msgpack.hpp>
#include "factory.h"
#include "res_output.h"
#include <fstream>
#include "imgui.h"
#if !defined(IMGUI_WAYLAND)
#include <GL/gl3w.h>
#else
#include"../../deps/glad/glad.h"
#endif
#include "af_shader.h"
#include "material.h"
void init_ui_component_by_mgo(base_ui_component*&ptar, msgpack::v2::object& obj)
{
	auto obj_arr_sz = obj.via.array.size;
	auto obj_typename = obj.via.array.ptr[0];
	auto obj_tp_nmlen = obj_typename.via.str.size;
	char* str_type_name = new char[obj_tp_nmlen + 1];
	memset(str_type_name, 0, obj_tp_nmlen + 1);
	memcpy(str_type_name, obj_typename.via.str.ptr, obj_tp_nmlen);
	ptar = factory::get().produce(str_type_name);
	delete[] str_type_name;
	vproperty_list vplist;
	int mlen=ptar->collect_property_range(vplist);
	auto mem_obj=obj.via.array.ptr[1];
	uint8_t* pmem = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(mem_obj.via.bin.ptr));
	for (auto& munit : vplist)
	{
		memcpy(munit._p_head_address, pmem, munit._len);
		pmem += munit._len;
	}
	if (obj_arr_sz>2)
	{
		auto childs_obj = obj.via.array.ptr[2];
		auto cd_sz = childs_obj.via.array.size;
		for (size_t ix = 0; ix < cd_sz; ix++)
		{
			auto cd_obj = childs_obj.via.array.ptr[ix];
			base_ui_component* pchild;
			init_ui_component_by_mgo(pchild, cd_obj);
			ptar->add_child(pchild);
		}
	}
}

void afb_load::load_afb(const char* afb_file)
{
	ifstream fin;
	fin.open(afb_file, ios::binary );
	if (!fin.is_open())
	{
		printf("invalid afb file:%s\n", afb_file);
	}
	auto file_size=fin.tellg();
	fin.seekg(0, ios::end);
	file_size = fin.tellg() - file_size;
	fin.seekg(0, ios::beg);
	msgpack::unpacker unpac;
	unpac.reserve_buffer(file_size);
	fin.read(unpac.buffer(), unpac.buffer_capacity());
	unpac.buffer_consumed(static_cast<size_t>(fin.gcount()));
	msgpack::object_handle oh;
	unpac.next(oh);
	auto obj_w = oh.get();
	base_ui_component::screenw = obj_w.via.array.ptr[0].as<float>();
	base_ui_component::screenh = obj_w.via.array.ptr[1].as<float>();
	auto obj_res = obj_w.via.array.ptr[2];
	auto re_cnt = obj_res.via.array.size;
	for (size_t ix = 0; ix < re_cnt; ix++)
	{
		g_vres_texture_list.emplace_back(res_texture_list());
		int cur_pos = g_vres_texture_list.size() - 1;
		res_texture_list& res_unit = g_vres_texture_list[cur_pos];
		glGenTextures(1, &res_unit.texture_id);
		glBindTexture(GL_TEXTURE_2D, res_unit.texture_id);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		// Step3 �趨filter����
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

		auto bin_res_unit = obj_res.via.array.ptr[ix];
		res_unit.texture_width = bin_res_unit.via.array.ptr[0].as<int>();
		res_unit.texture_height = bin_res_unit.via.array.ptr[1].as<int>();
		auto res_bin = bin_res_unit.via.array.ptr[2];
		auto bin_sz = res_bin.via.bin.size;
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, res_unit.texture_width, res_unit.texture_height,
			0, GL_RGBA, GL_UNSIGNED_BYTE, res_bin.via.bin.ptr);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
		auto res_data = bin_res_unit.via.array.ptr[3];
		auto res_data_sz = res_data.via.array.size;
		for (size_t iy = 0; iy < res_data_sz; iy++)
		{
			res_unit.vtexture_coordinates.emplace_back(res_texture_coordinate());
			int curpos = res_unit.vtexture_coordinates.size() - 1;
			res_texture_coordinate& cd_unit = res_unit.vtexture_coordinates[curpos];
			auto bin_cd_unit = res_data.via.array.ptr[iy];
			auto bin_filen = bin_cd_unit.via.array.ptr[0];
			cd_unit._file_name.reserve(bin_filen.via.str.size+1);
			cd_unit._file_name.resize(bin_filen.via.str.size+1);
			memcpy(&cd_unit._file_name[0], bin_filen.via.str.ptr, bin_filen.via.str.size);
			cd_unit._x0 = bin_cd_unit.via.array.ptr[1].as<float>();
			cd_unit._x1 = bin_cd_unit.via.array.ptr[2].as<float>();
			cd_unit._y0 = bin_cd_unit.via.array.ptr[3].as<float>();
			cd_unit._y1 = bin_cd_unit.via.array.ptr[4].as<float>();
		}

	}
	auto obj_shader_list = obj_w.via.array.ptr[3];
	auto shd_cnt = obj_shader_list.via.array.size;
	for (size_t ix = 0; ix < shd_cnt; ix++)
	{
		auto shd_unit = obj_shader_list.via.array.ptr[ix];
		auto shd_name = shd_unit.via.array.ptr[0];
		auto str_shd_nm_sz = shd_name.via.str.size;
		char* pshd_name = new char[str_shd_nm_sz + 1];
		memset(pshd_name, 0, str_shd_nm_sz + 1);
		memcpy(pshd_name, shd_name.via.str.ptr, str_shd_nm_sz);
		auto shd_vs_code = shd_unit.via.array.ptr[1];
		auto str_vs_code_sz = shd_vs_code.via.str.size;
		char* psd_vs_code = new char[str_vs_code_sz + 1];
		memset(psd_vs_code, 0, str_vs_code_sz + 1);
		memcpy(psd_vs_code, shd_vs_code.via.str.ptr, str_vs_code_sz);
		auto shd_fs_code = shd_unit.via.array.ptr[2];
		auto str_fs_code_sz = shd_fs_code.via.str.size;
		char* psd_fs_code = new char[str_fs_code_sz + 1];
		memset(psd_fs_code, 0, str_fs_code_sz + 1);
		memcpy(psd_fs_code, shd_fs_code.via.str.ptr, str_fs_code_sz);
		g_af_shader_list[pshd_name] = make_shared<af_shader>(psd_vs_code, psd_fs_code);
		delete[] pshd_name;
		delete[] psd_vs_code;
		delete[] psd_fs_code;
	}
	auto obj_material_list = obj_w.via.array.ptr[4];
	auto mtl_cnt = obj_material_list.via.array.size;
	for (size_t ix = 0; ix < mtl_cnt; ix++)
	{
		auto mtl_unit = obj_material_list.via.array.ptr[ix];
		auto mtl_name = mtl_unit.via.array.ptr[0];
		auto mtl_name_size = mtl_name.via.str.size;
		char* pstr_mtl_name = new char[mtl_name_size + 1];
		memset(pstr_mtl_name, 0, mtl_name_size + 1);
		memcpy(pstr_mtl_name, mtl_name.via.str.ptr, mtl_name_size);
		shared_ptr<material> pmtl = make_shared<material>();
		g_material_list[pstr_mtl_name] = pmtl;
		auto shd_name = mtl_unit.via.array.ptr[1];
		auto shd_name_size = shd_name.via.str.size;
		char* pstr_shd_name = new char[shd_name_size + 1];
		memset(pstr_shd_name, 0, shd_name_size+1);
		memcpy(pstr_shd_name, shd_name.via.str.ptr, shd_name_size);
		auto pshd = g_af_shader_list.find(pstr_shd_name);
		if (pshd != g_af_shader_list.end())
		{
			pmtl->set_pshader(pshd->second);
		}
		auto& mpshd_uf = pmtl->get_mp_sd_uf();
		auto shd_uf_list = mtl_unit.via.array.ptr[2];
		auto shd_uf_list_cnt = shd_uf_list.via.array.size;
		for (size_t iy = 0; iy < shd_uf_list_cnt; iy++)
		{
			auto shd_uf_unit = shd_uf_list.via.array.ptr[iy];
			auto shd_uf_name = shd_uf_unit.via.array.ptr[0];
			auto shd_uf_name_sz = shd_uf_name.via.str.size;
			char* pshd_uf_name = new char[shd_uf_name_sz + 1];
			memset(pshd_uf_name, 0, shd_uf_name_sz + 1);
			memcpy(pshd_uf_name, shd_uf_name.via.str.ptr, shd_uf_name_sz);
			auto shd_uf_type=shd_uf_unit.via.array.ptr[1];
			auto shd_uf_type_sz = shd_uf_type.via.str.size;
			char* pshd_uf_type = new char[shd_uf_type_sz + 1];
			memset(pshd_uf_type, 0, shd_uf_type_sz + 1);
			memcpy(pshd_uf_type, shd_uf_type.via.str.ptr, shd_uf_type_sz);

			auto shd_uf_usize=shd_uf_unit.via.array.ptr[2];
			auto shd_uf_el_size=shd_uf_unit.via.array.ptr[3];
			auto shd_uf_utype=shd_uf_unit.via.array.ptr[4];
			
			shared_ptr<shader_uf> pnunf = std::move(get_shader_uf_fc().Create(pshd_uf_type, shd_uf_usize.as<GLuint>(), shd_uf_el_size.as<GLuint>()));
			pnunf->set_type(shd_uf_utype.as<GLenum>());
			auto shd_data = shd_uf_unit.via.array.ptr[5];
			auto wsize = shd_uf_usize.as<GLuint>()*shd_uf_el_size.as<GLuint>();
			memcpy(pnunf->get_data_head(), shd_data.via.bin.ptr,wsize);
			mpshd_uf[pshd_uf_name] = pnunf;
			delete[] pshd_uf_name;
			delete[] pshd_uf_type;
		}
		delete[] pstr_mtl_name;
	}
	auto obj_ui = obj_w.via.array.ptr[5];
	init_ui_component_by_mgo(_pj, obj_ui);
}