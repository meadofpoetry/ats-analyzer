#ifndef ROOT_H
#define ROOT_H

#include <vector>
#include <gstreamermm.h>

#include "metadata.hpp"
#include "branch.hpp"

namespace Ats {

    class Root {
    public:
	Root(Root&) = delete;
	Root(const Root&&) = delete;
        ~Root();

	static unique_ptr<Root> create (const Glib::RefPtr<Gst::Bin>, const Metadata&);

	sigc::signal <void,std::shared_ptr <Pad> > signal_pad_added() { return _pad_added; }
	sigc::signal <void,std::shared_ptr <Pad> > signal_audio_pad_added() { return _audio_pad_added; }

    private:
	Root(const Glib::RefPtr<Gst::Bin>, const Metadata&);
	Glib::RefPtr<Gst::Bin> _bin;
	Glib::RefPtr<Gst::Element> _tee;
	Glib::RefPtr<Gst::Element> _demux;
	std::vector<std::unique_ptr<Branch>> _branches;
	
	void build_cb (const uint stream, const Meta_channel&);
	void build_branch (const uint, const uint, const Glib::RefPtr<Gst::Pad>&);
        void destroy_branch (const uint, const uint, const Glib::RefPtr<Gst::Pad>&);
	
	sigc::signal <void,std::shared_ptr <Pad> > _pad_added;
	sigc::signal <void,std::shared_ptr <Pad> > _audio_pad_added;
    };

}

#endif /* ROOT_H */
