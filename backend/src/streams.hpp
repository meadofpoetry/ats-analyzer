#ifndef STREAMS_H
#define STREAMS_H

#include <vector>
#include <string>
#include <gstreamermm.h>

#include "chatterer.hpp"
#include "metadata.hpp"

using namespace std;
using namespace Glib;

namespace Ats {

    using json = nlohmann::json;

    class Graph;
    class Probe;

    class Streams : public Chatterer, public Logger {

    public:
        /* ------- Prog list -------------------- */
        vector<Metadata> data;

        sigc::signal<void,const Streams&>   set;
        sigc::signal<void,const Streams&>   destructive_set;
        sigc::signal<void>                  updated;
        
        Streams(const std::string& n) : Chatterer(n) {}
        virtual ~Streams() {}

        bool   is_empty () const;
        void   set_data(const Metadata&);
        void   set_pid(const uint, const uint, const uint, Meta_pid::Pid_type);
        Metadata*           find_stream (uint stream);
        const Metadata*     find_stream (uint stream) const;

        // Chatter implementation
        string to_string() const;
        json   serialize() const;
        void   deserialize(const json&);

        void operator=(const Metadata& m) { set_data(m); }

        void   connect(Probe&);
        void   connect(Graph&);
    };
};

#endif /* STREAMS_H */