#include "probe.hpp"
#include "address.hpp"
#include "parser.hpp"

#include <cstdio>
#include <iostream>

#include <gst/gst.h>
#include <gst/mpegts/mpegts.h>
#include <glib.h>

using namespace Ats;

Probe::Probe(int s) : metadata(s) {
    stream = s;

    address a = get_address(s);

    auto src   = Gst::ElementFactory::create_element("udpsrc");
    auto parse = Gst::ElementFactory::create_element("tsparse");
    auto sink  = Gst::ElementFactory::create_element("fakesink");

    pipe   = Gst::Pipeline::create();

    pipe->add(src)->add(parse)->add(sink);

    src->link(parse)->link(sink);

    src->set_property("address", a.addr);
    src->set_property("port", a.port);
    src->set_property("timeout", 5000000000);

    bus    = pipe->get_bus();

    bus->add_watch(sigc::mem_fun(this, &Probe::on_bus_message));
}

/*
Probe::Probe(Probe&& src) : metadata(std::move(src.metadata)) {
    swap(pipe, src.pipe);
}
*/

void
Probe::set_state(Gst::State s) {
    pipe->set_state(s);
}

bool
Probe::on_bus_message(const Glib::RefPtr<Gst::Bus>& bus,
		      const Glib::RefPtr<Gst::Message>& msg) {
    GstMpegtsSection*   section;
  
    switch (msg->get_message_type()) {
    case Gst::MESSAGE_ELEMENT: {
	if ((msg->get_source()->get_name().substr(0, 11) == "mpegtsparse") &&
	    (section = gst_message_parse_mpegts_section (msg->gobj()))) {

	    if(Parse::table(section, metadata))
	        updated.emit(metadata);
	    
	    gst_mpegts_section_unref (section);
	} else {
	    if (msg->get_structure().get_name() == "GstUDPSrcTimeout") {

		if (! metadata.is_empty()) {
		    metadata.clear();
		    pipe->set_state(Gst::STATE_NULL);
		    pipe->set_state(Gst::STATE_PLAYING);
		    updated.emit(metadata);
		}
	    }
	}
    }
	break;
    default: break;
    }
    return    true;
}