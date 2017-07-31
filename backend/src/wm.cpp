#include "wm.hpp"

using namespace std;
using namespace Ats;

void /* TODO err */
Wm::add_to_pipe(Glib::RefPtr<Gst::Bin> b) {
    _bin        = b;
    _mixer      = Gst::ElementFactory::create_element("glvideomixer");
    _background = Gst::ElementFactory::create_element("videotestsrc");
    _background->set_property("is_live", true);

    _bin->add(_mixer)->add(_background);
    
    _background_pad = _mixer->get_request_pad("sink_%u");

    auto in_pad = _background->get_static_pad("src");
    in_pad->link(_background_pad);
}

void
Wm::plug(shared_ptr<Pad> src) {
    switch (src->type()) {
    case Pad::Type::Video: {
	auto w = shared_ptr<Wm_window> (new Wm_window ());
	// TODO try catch
	w->plug(src);
	auto pos  = make_pair(src->stream(),src->pid());
	auto wres = _windows.try_emplace(pos,w);
	if (wres.second) { // inserted
	    w->add_to_pipe(_bin);
	    auto sink_pad = _mixer->get_request_pad("sink_%u");
	    w->plug(sink_pad);
	}
	break;
    }
    case Pad::Type::Graph_volume:
    case Pad::Type::Audio:
    case Pad::Type::Unknown:
	break;
    }
}

void
Wm::plug (Glib::RefPtr<Gst::Pad> sink) {
    _mixer->get_static_pad("src")->link(sink);
}

void
Wm::on_remove_sink(uint stream, uint pid) {

}
