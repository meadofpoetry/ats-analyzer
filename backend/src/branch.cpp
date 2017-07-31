#include <glibmm.h>

#include "branch.hpp"

using namespace std;
using namespace Ats;

unique_ptr<Branch>
Branch::create(std::string type, uint stream, uint chan, uint pid) {

    if (type == "video") return unique_ptr<Branch>((Branch*) new Video_branch(stream, chan, pid));
    else if (type == "audio") return unique_ptr<Branch>((Branch*) new Audio_branch(stream, chan, pid));
    else return unique_ptr<Branch>(nullptr);
}

Branch::Branch() {
    _bin    = Gst::Bin::create();
    auto queue   = Gst::ElementFactory::create_element("queue");
    _decoder = Gst::ElementFactory::create_element("decodebin");

    queue->set_property("max-size-buffers", 20000);
    queue->set_property("max-size-bytes", 12000000);

    _bin->add(queue)->add(_decoder);
    queue->link(_decoder);
}

void
Branch::connect_src ( const Glib::RefPtr<Gst::Pad>& p ) {
    auto sink_pad  = _bin->get_static_pad("sink");
    p->link(sink_pad);
    _bin->sync_state_with_parent();
}

Video_branch::Video_branch(uint stream, uint chan, uint pid) {

    _decoder->signal_pad_added().
	connect([this,stream,chan,pid](const Glib::RefPtr<Gst::Pad>& pad)
		{
		    auto pcaps = pad->get_current_caps()->get_structure(0).get_name();
		    vector<Glib::ustring> caps_toks = Glib::Regex::split_simple("/", pcaps);
		    auto& type    = caps_toks[0];

		    if (type != "video") return;

		    auto deint  = Gst::ElementFactory::create_element("deinterlace");
		    auto _analyser = Gst::ElementFactory::create_element("videoanalysis");
		    
		    auto sink_pad = deint->get_static_pad("sink");
		    auto src_pad  = _analyser->get_static_pad("src");

		    _bin->add(deint)->add(_analyser);
		    deint->link(_analyser);

		    pad->link(sink_pad);

		    auto src_ghost = Gst::GhostPad::create(src_pad, "src");
		    src_ghost->set_active();
		    _bin->add_pad(src_ghost);

		    auto p = shared_ptr<Pad>(new Pad(stream,
						     chan, pid,
						     "video",
						     (Glib::RefPtr<Gst::Pad>) src_ghost));

		    _pads.push_back(p);
		    _pad_added.emit(p);
		});
    
}

Audio_branch::Audio_branch(uint stream, uint chan, uint pid) {

}


/* 
   TODO: MOVE TO WINDOW 
   void
   Graph::build_subbranch(RefPtr<Gst::Bin> bin, uint stream, uint channel, uint pid, const RefPtr<Gst::Pad>& p) {
    RefPtr<Gst::Pad> src_pad;

    auto set_video = [this,&p,stream,channel,pid](){
	Meta_pid::Video_pid v;

	auto vi = gst_video_info_new();
	auto pcaps = p->get_current_caps();
	if (! gst_video_info_from_caps(vi, pcaps->gobj())) {
	    gst_video_info_free(vi);
	    return;
	}
			
	v.codec = "h264"; // FIXME not only h264 supported
	v.width = vi->width;
	v.height = vi->height;
	v.aspect_ratio = {vi->par_n,vi->par_d};
	v.frame_rate = (float)vi->fps_n/vi->fps_d;

	gst_video_info_free(vi);

	Meta_pid::Pid_type rval = v;

	set_pid.emit(stream,channel,pid,rval);
    };

    auto pcaps = p->get_current_caps()->get_structure(0).get_name();
    vector<Glib::ustring> caps_toks = Glib::Regex::split_simple("/", pcaps);
    auto& type    = caps_toks[0];	    

    if (type == "video") {
	// Video callback
	p->connect_property_changed("caps", set_video);
	set_video();
		
	auto _deint  = Gst::ElementFactory::create_element("deinterlace");
	auto anal    = Gst::ElementFactory::create_element("videoanalysis");
	auto _scale  = Gst::ElementFactory::create_element("videoscale");
	auto _caps   = Gst::ElementFactory::create_element("capsfilter");

	_caps->set_property("caps", Gst::Caps::create_from_string("video/x-raw,pixel-aspect-ratio=1/1"));

	
//	n.type = type;
//	n.analysis = anal;
	
			
	auto _sink = _deint->get_static_pad("sink");
	src_pad = _caps->get_static_pad("src");
		
	bin->add(_deint)->add(anal)->add(_scale)->add(_caps);
	_deint->link(anal)->link(_scale)->link(_caps);
		
	_deint->sync_state_with_parent();
	anal->sync_state_with_parent();
	_scale->sync_state_with_parent();
	_caps->sync_state_with_parent();
		
	p->link(_sink);
    } else if (type == "audio") {
	src_pad = p;
    } else {
	return;
    }
    auto src_ghost = Gst::GhostPad::create(src_pad, "src");
    src_ghost->set_active();
            bin->add_pad(src_ghost);
}
*/