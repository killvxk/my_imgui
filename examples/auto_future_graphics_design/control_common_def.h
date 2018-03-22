#pragma once
#include <imgui.h>
#include <vector>
#include <string>
#include "json/json.h"
//#include <map>
struct ft_vertex
{

	ImVec4  pos;
	ImVec2  uv;
	ImU32   col;
};
typedef unsigned short ushort;
using namespace std;
using namespace Json;
class base_ui_component;
extern base_ui_component* find_a_uc_from_uc(base_ui_component& tar_ui, const char* uname);
class base_ui_component
{
	friend base_ui_component* find_a_uc_from_uc(base_ui_component& tar_ui, const char* uname);
protected:
	string _name;
	int _texture_id_index;//note:the field is not texture id(texture id is generated by glGenTextures)
	vector<base_ui_component*> _vchilds;
	base_ui_component* _parent;
	bool _visible;
#if !defined(IMGUI_WAYLAND)
protected:
	bool _selected;
public:
	virtual void draw_peroperty_page() = 0;
	bool is_selected()
	{
		return _selected;
	}
	void set_selected(bool beselected)
	{
		_selected = beselected;
	}
#endif
public:
	virtual void draw() = 0;
	//virtual base_ui_component* get_new_instance() = 0;
	void set_name(string& name){ _name = name; }
	string& get_name(){ return _name; }
	virtual bool handle_mouse(){ return false; }
	virtual bool init_from_json(Value& ){ return true; }
	base_ui_component()
		:_texture_id_index(0)
		,_visible(true)
		, _parent(NULL)
#if !defined(IMGUI_WAYLAND)
		, _selected(false)
#endif
	{}
	
	~base_ui_component()
	{
		for (auto it:_vchilds)
		{
			delete it;
		}
	}
	void add_child(base_ui_component* pchild){ pchild->_parent = this; _vchilds.push_back(pchild); }
	base_ui_component* get_child(int index)
	{
		return _vchilds[index];
	}
	size_t get_child_count(){ return _vchilds.size(); }
	base_ui_component* get_parent(){ return _parent; }
	size_t child_count(){ return _vchilds.size(); }
	virtual void offset(ImVec2& imof)
	{
		for (auto it:_vchilds)
		{
			it->offset(imof);
		}
	}
	int get_texture_id_index(){return _texture_id_index; }
	bool is_visible(){ return _visible; }
	void set_visible(bool visible){ _visible = visible; }

};
