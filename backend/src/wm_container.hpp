#ifndef WM_CONTAINER_H
#define WM_CONTAINER_H

#include "wm_widget.hpp"
#include "wm_window.hpp"

#include <functional>
#include <map>

namespace Ats {

    class Wm_container {
    public:
        Wm_container (std::shared_ptr<Wm_window> w) : _name(w->gen_name()), _window(w) { _window->enable(); }
        Wm_container (Wm_container&) = delete;
        Wm_container (const Wm_container&&) = delete;
        ~Wm_container ();
	
        void add_widget (shared_ptr<Wm_widget>);
        void remove_widget (std::string);

        void validate () {}

        void apply    (std::function<void(Wm_window&)>&);
        void apply    (std::function<void(const Wm_window&)>& f) const;
        void for_each (std::function<void(const std::string&,Wm_widget&)>&);
        void for_each (std::function<void(const std::string&,const Wm_widget&)>& f) const;
    private:
        std::string _name;
        std::shared_ptr<Wm_window>                       _window;
        std::map<std::string,std::shared_ptr<Wm_widget>> _widgets;
    };
	
}

#endif /* WM_CONTAINER_H */
