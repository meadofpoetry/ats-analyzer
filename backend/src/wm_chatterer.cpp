#include "wm.hpp"

using namespace std;
using namespace Ats;

string
Wm::to_string() const {
    std::string rval = "";
    auto position_to_string = [](const Wm_position& p) {
        std::string rval;
        rval += "\nPosition: ";
        rval += "left: ";
        rval += std::to_string(p.get_left());
        rval += ", top: ";
        rval += std::to_string(p.get_top());
        rval += ", right: ";
        rval += std::to_string(p.get_right());
        rval += ", bottom: ";
        rval += std::to_string(p.get_bottom());
        return rval;
    };
    auto window_to_string = [&position_to_string](const Wm_window* w){
        std::string rval;
        rval += "\nType: ";
        rval += w->get_type_string();
        rval += "\nIs enabled: ";
        rval += Ats::to_string(w->is_enabled());
        if (w->type() == Wm_window::Type::Video) {
            const Wm_window_video* vw = dynamic_cast<const Wm_window_video*>(w);
            rval += "\nStream: ";
            rval += std::to_string(vw->stream());
            rval += "\nChannel: ";
            rval += std::to_string(vw->channel());
            rval += "\nPID: ";
            rval += std::to_string(vw->pid());
        }
        else {
            /* TODO */
        }
        rval += position_to_string(w->get_position());
        return rval;
    };
    auto widget_to_string = [&position_to_string](const Wm_widget* w) {
        std::string rval;
        rval += "\nType: ";
        rval += w->get_type_string();
        rval += "\nIs enabled: ";
        rval += Ats::to_string(w->is_enabled());
        /* TODO type dependent things */
        rval += position_to_string(w->get_position());
        return rval;
    };

    // resolution
    rval += "\nResolution: ";
    rval += std::to_string(_resolution.first);
    rval += "x";
    rval += std::to_string(_resolution.second);
    // windows
    rval += "\n\n-------------- Windows ---------------";
    std::string windows;
    for (auto it = _windows.begin(); it != _windows.end(); ++it) {
        windows += "\n\nUID: ";
        windows += it->first;
        windows += window_to_string(it->second.get());
        auto end_it = _windows.end();
        -- end_it;
        if (it != end_it) windows += "\n";
    }
    rval += Ats::add_indent(windows,1);
    // widgets
    rval += "\n\n-------------- Widgets ---------------";
    std::string widgets;
    for (auto it = _widgets.begin(); it != _widgets.end(); ++it) {
        widgets += "\n\nUID: ";
        widgets += it->first;
        widgets += widget_to_string(it->second.get());
    }
    rval += Ats::add_indent(widgets,1);
    // layout
    rval += "\n\n-------------- Layout ----------------";
    std::string layout;
    std::function<void(const std::string&,const Wm_container&)> f =
        [&layout,&window_to_string,&widget_to_string](const std::string& s, const Wm_container& c) {
        layout += "\n\nUID: ";
        layout += s;
        layout += window_to_string(c.get_window().get());
        layout += "\nWidgets: ";
        std::string layout_widgets;
        std::function<void(const std::string&,const Wm_widget&)> f_wdg =
        [&layout_widgets,&widget_to_string](const std::string& s_wdg, const Wm_widget& wdg) {
            layout_widgets += widget_to_string(&wdg);
        };
        c.for_each(f_wdg);
        layout += Ats::add_indent(layout_widgets,1);
    };
    _treeview.for_each(f);
    rval += Ats::add_indent(layout,1);
    rval += "\n\n--------------------------------------";

    return rval;
}

json
Wm::serialize() const {
    typedef std::function<void(const std::string&,const Wm_container&)> f_containers_t;

    std::vector<pair<std::string,shared_ptr<const Wm_window>>> windows_v(_windows.begin(), _windows.end());
    std::vector<pair<std::string,shared_ptr<const Wm_widget>>> widgets_v(_widgets.begin(), _widgets.end());
    json j_windows(windows_v);
    json j_widgets(widgets_v);

    json j_treeview;
    f_containers_t f = [&j_treeview](const std::string& s,const Wm_container& c) {
        json j_container = {s,c};
        j_treeview.push_back(j_container);
    };
    _treeview.for_each(f);

    json j = json{{"background",{}},
                  {"resolution",{_resolution.first,_resolution.second}},
                  {"windows",j_windows},
                  {"widgets",j_widgets},
                  {"layout",j_treeview}};
    return j;
}

void
Wm::deserialize(const json& j) {
    constexpr const char* wm_bg_key         = "background";
    constexpr const char* wm_resolution_key = "resolution";
    constexpr const char* wm_layout_key     = "layout";

    if (j.find(wm_bg_key) != j.end()) {
        const json j_background = j.at(wm_bg_key);
        /* TODO */
    }
    if (j.find(wm_resolution_key) != j.end()) {
        const json j_resolution = j.at(wm_resolution_key);
        uint width  = j_resolution.at(0).get<uint>();
        uint height = j_resolution.at(1).get<uint>();
        const resolution_t p(width,height);
        set_resolution(p);
    }
    if (j.find(wm_layout_key) != j.end()) {
        const json j_layout = j.at(wm_layout_key);
        unique_ptr<Wm_treeview_template> tw_template = Wm_treeview_template::create(j_layout,_windows,_widgets);
        tw_template->validate(_resolution);
        _treeview.reset_from_template(tw_template.get());
    }

}

void
Ats::to_json(json& j, const shared_ptr<const Wm_window> w) {
    /* Window type-independent fields */
    j = {{"enabled",w->is_enabled()},
         {"position",w->get_position()}};

    /* Window type-dependent fields */
    Wm_window::Type t = w->type();
    if (t == Wm_window::Type::Video) {
        const Wm_window_video* wv = dynamic_cast<const Wm_window_video*> (w.get());
        j["type"]    = "video";
        j["stream"]  = wv->stream();
        j["channel"] = wv->channel();
        j["pid"]     = wv->pid();
    }
    else {
        j["type"]    = "background";
    }
}

void
Ats::to_json(json& j, const shared_ptr<const Wm_widget> w) {
    /* Widget type-independent fields */
    j = {{"enabled",w->is_enabled()},
         {"position",w->get_position()}};

    /* Widget type-dependent fields */
    /* TODO */
}

void
Ats::to_json(json& j, const Wm_container& c) {
    json j_widgets;
    std::function<void(const std::string&,const Wm_widget&)> f_widgets =
        [&j_widgets](const std::string& s, const Wm_widget& w) {
        json j_widget = {s,shared_ptr<const Wm_widget>(&w)};
        j_widgets.push_back(j_widget);
    };

    j = c.get_window();
    c.for_each(f_widgets);
    j["widgets"] = j_widgets;
}

void
Ats::to_json(json& j, const Wm_position& p) {
    j = {{"left",p.get_left()},
         {"top",p.get_top()},
         {"right",p.get_right()},
         {"bottom",p.get_bottom()}};
}
