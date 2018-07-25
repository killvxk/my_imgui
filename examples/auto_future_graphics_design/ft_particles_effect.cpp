﻿#include "ft_particles_effect.h"
#include <fstream>
#include "SOIL.h"
#include "texture.h"
#include <GLFW/glfw3.h>
#include "paricles_system.h"
#include <chrono>
#include <random>
const int MaxParticles = 100000;

namespace auto_future
{
	GLfloat* get_txt_uvs(const char* data_file, int& retn_len)
	{
		GLfloat* puvs = NULL;
		Reader reader;
		Value jvalue;
		if (reader.parse(data_file, jvalue, false))
		{
			Value& meta = jvalue["meta"];
			Value& jsize = meta["size"];
			float w = jsize["w"].asInt();
			float h = jsize["h"].asInt();
			Value& frames = jvalue["frames"];
			Value::Members memb(frames.getMemberNames());
			retn_len = memb.size() * 8;
			puvs = new GLfloat[retn_len];
			int idx = 0;
			for (auto itmem = memb.begin(); itmem != memb.end(); ++itmem, ++idx)
			{
				auto& mname = *itmem;
				Value& junif = frames[mname];
				Value& frame = junif["frame"];
				int sbidx = idx * 8;
				float x0 = frame["x"].asInt();
				float y0 = frame["y"].asInt();
				float x1 = x0 + frame["w"].asInt();
				float y1 = y0 + frame["h"].asInt();

				puvs[sbidx + 0] = x0 / w;
				puvs[sbidx + 1] = y0 / h;

				puvs[sbidx + 2] = x1 / w;
				puvs[sbidx + 3] = y0 / h;
				puvs[sbidx + 4] = x0 / w;
				puvs[sbidx + 5] = y1 / h;
				puvs[sbidx + 6] = x1 / w;
				puvs[sbidx + 7] = y1 / h;
				/*puvs[sbidx + 0] = x1 / w;
				puvs[sbidx + 1] = y0 / h;
				puvs[sbidx + 2] = x1 / w;
				puvs[sbidx + 3] = y1 / h;
				puvs[sbidx + 4] = x0 / w;
				puvs[sbidx + 5] = y0 / h;
				puvs[sbidx + 6] = x1 / w;
				puvs[sbidx + 7] = y0 / h;*/

			}
		}
		return puvs;
	}
	static const GLfloat g_vertex_cc_data[] = {
		-0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f,
		0.5f, 0.5f, 0.0f,
	};
		
	static default_random_engine generator;
	static uniform_real_distribution<float> distribution(-5.f, 5.f);
	static uniform_real_distribution<float> distribution1(-3.f, 0.f);
	//#define _calcu_color
	static unsigned char ptclsize[] =
	{
		0, 1, 2, 3, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 12, 12, 13, 13, 14, 14, 15, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16, 17, 17, 17, 18, 18, 18, 19, 19, 19, 20, 20, 21, 21, 22, 22, 23, 23, 24, 24, 25, 26, 27, 28, 29,
	};

	static shared_ptr<GPSystem> g_ptcl_sys;
	ft_particles_effect::ft_particles_effect()
	{
		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);
		auto& mut = g_material_list.find("particles1");
		_particle_material = mut->second;
		float cmr_wp[] = { 0.999137f, -0.000177f, 0.041540f };
		_particle_material->set_value("CameraRight_worldspace", cmr_wp, 3);
		//glUniform3f(CameraRight_worldspace_ID, ViewMatrix[0][0], ViewMatrix[1][0], ViewMatrix[2][0]);
		float cmu_wp[] = { -0.009298f, 0.973666f, 0.227788f };
		_particle_material->set_value("CameraUp_worldspace", cmu_wp, 3);
		//glUniform3f(CameraUp_worldspace_ID, ViewMatrix[0][1], ViewMatrix[1][1], ViewMatrix[2][1]);
		float vwpj[] = { 1.809097f, -0.022448f, 0.040568f, 0.040486f,
			-0.000320f, 2.350639f, 0.228434f, 0.227978f,
			0.075215f, 0.549929f, -0.974772f, -0.972824f,
			-0.376075f, -2.749644f, 4.673659f, 4.864121f };
		_particle_material->set_value("VP", vwpj, 16);
		auto& muf = g_mfiles_list.find("flame_fire.js");
		auto _pfile = muf->second;
		int rlen;
		GLfloat* puvs = get_txt_uvs((char*)_pfile->_pbin, rlen);
		_particle_material->set_value("uvcol[0]", puvs, rlen);
		auto& mtx = g_mtexture_list.find("flame_fire1.png");
		_texture = mtx->second;
		

		glGenBuffers(1, &_vbo_uv);
		glBindBuffer(GL_ARRAY_BUFFER, _vbo_uv);
		glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_cc_data), g_vertex_cc_data, GL_STATIC_DRAW);
		glGenBuffers(1, &_vbo_pos);
		glBindBuffer(GL_ARRAY_BUFFER, _vbo_pos);
		glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
		glGenBuffers(1, &_vbo_color);
		glBindBuffer(GL_ARRAY_BUFFER, _vbo_color);
		glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW);

		lastTime = glfwGetTime();
		g_ptcl_sys = make_shared<GPSystem>();
		g_ptcl_sys->AddForce(make_shared<Gravity>());
		g_ptcl_sys->AddForce(make_shared<GViscosity>());
		g_ptcl_sys->create_paricle(MaxParticles);
		g_ptcl_sys->add_particle_set([this](GParticle* pcl){
			GParticle& p = *pcl;
			p._size = 0;

			switch (_pt._pa)
			{
			case en_fire:
			{
				p._pos.x = _pt._pos0.x;
				p._pos.y = _pt._pos0.y;
				p._pos.z = _pt._pos0.z;
				p._life = _pt._life;
				p._vel.x = distribution(generator);
				p._vel.y = distribution1(generator);
				p._vel.z = 0;// distribution1(generator);
			}
			break;
			case en_fire_with_smoke:
			{
				p._pos.z = _pt._pos0.z;
				p._life = 2.f;
				p._pos.x = _pt._pos0.x + distribution(generator);
				p._pos.y = _pt._pos0.y + distribution1(generator);
				glm::vec3 postg(_pt._pos0.x, _pt._pos0.y - _pt._y1, _pt._pos0.z);
				p._vel = postg - p._pos;
			}
			break;
			case en_fountain:
				p._life = (((rand() % 125 + 1) / 10.0) + 5);
				p._pos.x = (rand() % 30);
				p._pos.y = _pt._pos0.y;
				p._pos.z = _pt._pos0.z;
				p._vel.x = (((((((2) * rand() % 11) + 1)) * rand() % 11) + 1) * 0.005) - (((((((2) * rand() % 11) + 1)) * rand() % 11) + 1) * 0.005);
				p._vel.y = ((((((5) * rand() % 11) + 5)) * rand() % 11) + 10) * 0.02;
				p._vel.z = (((((((2) * rand() % 11) + 1)) * rand() % 11) + 1) * 0.005) - (((((((2) * rand() % 11) + 1)) * rand() % 11) + 1) * 0.005);

				break;

			default:
			{
				p._life = _pt._life; // This particle will live 5 seconds.
				p._pos = glm::vec3(_pt._pos0.x, _pt._pos0.y, _pt._pos0.z);
				glm::vec3 maindir = glm::vec3(_pt._v0.x, _pt._v0.y, _pt._v0.z);
				glm::vec3 randomdir = glm::vec3(
					(rand() % 2000 - 1000.0f) / 1000.0f,
					(rand() % 2000 - 1000.0f) / 1000.0f,
					(rand() % 2000 - 1000.0f) / 1000.0f
					);
				p._vel = maindir + randomdir*_pt._spread;
				p._size = rand() % 30;// (rand() % 1000) / 2000.0f + 0.1f;
			}
			break;
			}
			//p._size = 0;
		});
	}


	ft_particles_effect::~ft_particles_effect()
	{
	}

	void ft_particles_effect::draw()
	{
		double currentTime = glfwGetTime();
		double delta = currentTime - lastTime;
		lastTime = currentTime;
		int newparticles = (int)(delta*10000.0);
		if (newparticles > (int)(0.016f*10000.0))
			newparticles = (int)(0.016f*10000.0);
		g_ptcl_sys->spawn_particles(newparticles);

		//printf("delta=%f\n", delta);
		g_ptcl_sys->EulerStep(float(delta));

		//SortParticles();
		//printf("particles count:%d\n", ParticlesCount);
		// Use our shader
		_particle_material->use();
		glBindVertexArray(_vao);
		glBindBuffer(GL_ARRAY_BUFFER, _vbo_pos);
		glBufferData(GL_ARRAY_BUFFER, g_ptcl_sys->valid_data_count() * sizeof(GLfloat), g_ptcl_sys->get_pPos_data(), GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. See above link for details.
		//glBufferSubData(GL_ARRAY_BUFFER, 0, ParticlesCount * sizeof(GLfloat) * 4, g_particule_position_size_data);

#ifdef _calcu_color
		glBindBuffer(GL_ARRAY_BUFFER, _vbo_color);
		glBufferData(GL_ARRAY_BUFFER, g_ptcl_sys->valid_data_count() * sizeof(GLubyte), g_ptcl_sys->get_pCol_data(), GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. See above link for details.
		//glBufferSubData(GL_ARRAY_BUFFER, 0, ParticlesCount * sizeof(GLubyte) * 4, g_particule_color_data);
#endif
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, _texture->_txt_id);
		glUniform1i(_texture->_txt_id, 0);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, _vbo_uv);
		glVertexAttribPointer(
			0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);
		glVertexAttribDivisor(0, 0); // particles vertices : always reuse the same 4 vertices -> 0

		// 2nd attribute buffer : positions of particles' centers
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, _vbo_pos);
		glVertexAttribPointer(
			1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
			4,                                // size : x + y + z + size => 4
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
			);
		glVertexAttribDivisor(1, 1); // positions : one per quad (its center)                 -> 1

#ifdef _calcu_color
		// 3rd attribute buffer : particles' colors
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, _vbo_color);
		glVertexAttribPointer(
			2,                                // attribute. No particular reason for 1, but must match the layout in the shader.
			4,                                // size : r + g + b + a => 4
			GL_UNSIGNED_BYTE,                 // type
			GL_TRUE,                          // normalized?    *** YES, this means that the unsigned char[4] will be accessible with a vec4 (floats) in the shader ***
			0,                                // stride
			(void*)0                          // array buffer offset
			);
		glVertexAttribDivisor(2, 1); // color : one per quad                                  -> 1
#endif

		// Draw the particules !
		// This draws many times a small triangle_strip (which looks like a quad).
		// This is equivalent to :
		// for(i in ParticlesCount) : glDrawArrays(GL_TRIANGLE_STRIP, 0, 4), 
		// but faster.
		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, g_ptcl_sys->valid_particle_cnt());
		//glDrawArraysInstanced(GL_TRIANGLES, 0, 6, ParticlesCount);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
	}
#if !defined(IMGUI_DISABLE_DEMO_WINDOWS)
	void ft_particles_effect::draw_peroperty_page()
	{
		ImGui::Text("Emitting position:");
		ImGui::SliderFloat("px", &_pt._pos0.x, -100.f, 100.f);
		ImGui::SliderFloat("py", &_pt._pos0.y, -100.f, 100.f);
		ImGui::SliderFloat("pz", &_pt._pos0.z, -100.f, 100.f);
		ImGui::Text("Emitting velocity:");
		ImGui::SliderFloat("vx", &_pt._v0.x, -100.f, 100.f);
		ImGui::SliderFloat("vy", &_pt._v0.y, -100.f, 100.f);
		ImGui::SliderFloat("vz", &_pt._v0.z, -100.f, 100.f);
		ImGui::Text("Accelerated velocity:");
		ImGui::SliderFloat("ax", &_pt._a0.x, -100.f, 100.f);
		ImGui::SliderFloat("ay", &_pt._a0.y, -100.f, 100.f);
		ImGui::SliderFloat("az", &_pt._a0.z, -100.f, 100.f);
		ImGui::Text("Life(seconds):");
		ImGui::SliderFloat("life", &_pt._life, 0.f, 20.f);
		ImGui::SliderFloat("spread", &_pt._spread, 0.f, 50.f);
		ImGui::SliderFloat("y1", &_pt._y1, -40.f, 40);
		static const char* a_show[]{"normal", "gravity", "fountain", "fire", "fire with smoke", };
		ImGui::Combo("particles type:", &_pt._pa, a_show, en_alg_cnt);
		g_ptcl_sys->draw_property();

	}
	bool ft_particles_effect::init_from_json(Value& jvalue)
	{
		return true;
	}
	bool ft_particles_effect::init_json_unit(Value& junit)
	{
		return true;
	}
#endif
}