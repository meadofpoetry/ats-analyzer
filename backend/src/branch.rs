use std::sync::{Arc,Mutex};
use gst::prelude::*;
use gst;
use std::str::Split;
use pad::SrcPad;
use signals::Signal;

struct CommonBranch {
    decoder: gst::Element,
    bin: gst::Bin,
    sink: gst::Pad,
}

impl CommonBranch {
    pub fn new () -> CommonBranch {
        let bin   = gst::Bin::new(None);
        let queue = gst::ElementFactory::make("queue", None).unwrap();

        for el in &["vaapih264dec", "vaapimpeg2dec", "vaapidecodebin"] {
            gst::ElementFactory::find(&el).map(|f| f.set_rank(0));
        };

        let decoder = gst::ElementFactory::make("decodebin",None).unwrap();

        queue.set_property("max-size-buffers", &20000);
        queue.set_property("max-size-bytes", &12000000);
        
        bin.add_many(&[&queue, &decoder]).unwrap();
        queue.link(&decoder).unwrap();

        let p = queue.get_static_pad("sink").unwrap();
        let sink_ghost = gst::GhostPad::new(Some("sink"), &p).unwrap();

        sink_ghost.set_active(true).unwrap();
        bin.add_pad(&sink_ghost).unwrap();

        CommonBranch { decoder, bin, sink: sink_ghost.upcast() }
    }
}

#[derive(Clone)]
pub struct VideoBranch {
    stream:   u32,
    channel:  u32,
    pid:      u32,
    pads:     Arc<Mutex<Vec<SrcPad>>>,
    analyser: gst::Element,
    decoder:  gst::Element,
    bin:      gst::Bin,
    sink:     gst::Pad,
    
    pub pad_added: Arc<Mutex<Signal<SrcPad>>>,
}

impl VideoBranch {
    pub fn new (stream: u32, channel: u32, pid: u32) -> VideoBranch {
        let common = CommonBranch::new();
        
        let pads      = Arc::new(Mutex::new(Vec::new()));
        let analyser  = gst::ElementFactory::make("videoanalysis",None).unwrap();
        let decoder   = common.decoder;
        let bin       = common.bin;
        let sink      = common.sink;
        let pad_added = Arc::new(Mutex::new(Signal::new()));

        let bin_c = bin.clone();
        let pads_c = pads.clone();
        let analyser_c = analyser.clone();
        let pad_added_c = pad_added.clone();
        
        decoder.connect_pad_added(move | _, pad | {
            let pcaps = String::from(pad.get_current_caps().unwrap().get_structure(0).unwrap().get_name());
            let caps_toks: Vec<&str> = pcaps.split('/').collect();
            if caps_toks.len() == 0 { return };

            let typ = &caps_toks[0];

            if *typ != "video" { return };

            let deint = gst::ElementFactory::make("deinterlace",None).unwrap();

            let sink_pad = deint.get_static_pad("sink").unwrap();
            let src_pad  = analyser_c.get_static_pad("src").unwrap();

            bin_c.add_many(&[&deint, &analyser_c]).unwrap();
            deint.link(&analyser_c).unwrap();

            deint.sync_state_with_parent();
            analyser_c.sync_state_with_parent();

            pad.link(&sink_pad);

            let spad = SrcPad::new(stream, channel, pid, "video", bin_c.clone(), src_pad);

            pad_added_c.lock().unwrap().emit(&spad);
            pads_c.lock().unwrap().push(spad);
        });
        
        VideoBranch { stream, channel, pid, pads, analyser, decoder, bin, sink, pad_added }
    }

    pub fn plug(&self, src_pad: &gst::Pad) {
        src_pad.link(&self.sink);
        self.bin.sync_state_with_parent();
    }

    pub fn add_to_pipe (&self, b: &gst::Bin) {
        b.add(&self.bin);
        self.bin.sync_state_with_parent();
    }
}

#[derive(Clone)]
pub struct AudioBranch {
    stream:   u32,
    channel:  u32,
    pid:      u32,
    pads:     Arc<Mutex<Vec<SrcPad>>>,
    analyser: gst::Element,
    decoder:  gst::Element,
    bin:      gst::Bin,
    sink:     gst::Pad,
    
    pub pad_added: Arc<Mutex<Signal<SrcPad>>>,
    pub audio_pad_added: Arc<Mutex<Signal<SrcPad>>>,
}

impl AudioBranch {
    pub fn new (stream: u32, channel: u32, pid: u32) -> AudioBranch {
        let common = CommonBranch::new();
        
        let pads      = Arc::new(Mutex::new(Vec::new()));
        let analyser  = gst::ElementFactory::make("audioanalysis",None).unwrap();
        let decoder   = common.decoder;
        let bin       = common.bin;
        let sink      = common.sink;
        let pad_added = Arc::new(Mutex::new(Signal::new()));
        let audio_pad_added = Arc::new(Mutex::new(Signal::new()));
        
        let bin_c = bin.clone();
        let pads_c = pads.clone();
        let analyser_c = analyser.clone();
        let pad_added_c = pad_added.clone();
        let audio_pad_added_c = audio_pad_added.clone();
        
        decoder.connect_pad_added(move | _, pad | {
            let pcaps = String::from(pad.get_current_caps().unwrap().get_structure(0).unwrap().get_name());
            let caps_toks: Vec<&str> = pcaps.split('/').collect();
            if caps_toks.len() == 0 { return };

            let typ = &caps_toks[0];

            if *typ != "audio" { return };

            let deint = gst::ElementFactory::make("deinterlace",None).unwrap();

            let sink_pad = deint.get_static_pad("sink").unwrap();
            let src_pad  = analyser_c.get_static_pad("src").unwrap();

            bin_c.add_many(&[&deint, &analyser_c]).unwrap();
            deint.link(&analyser_c).unwrap();

            deint.sync_state_with_parent();
            analyser_c.sync_state_with_parent();

            pad.link(&sink_pad);

            let spad = SrcPad::new(stream, channel, pid, "audio", bin_c.clone(), src_pad);

            pad_added_c.lock().unwrap().emit(&spad);
            audio_pad_added_c.lock().unwrap().emit(&spad);
            pads_c.lock().unwrap().push(spad);
        });

        bin.sync_children_states();
        AudioBranch { stream, channel, pid, pads, analyser, decoder, bin, sink, pad_added, audio_pad_added }
    }

    pub fn plug(&self, src_pad: &gst::Pad) {
        src_pad.link(&self.sink);
        self.bin.sync_state_with_parent();
    }

    pub fn add_to_pipe (&self, b: &gst::Bin) {
        b.add(&self.bin);
        self.bin.sync_state_with_parent();
    }
}

#[derive(Clone)]
pub enum Branch {
    Video(VideoBranch),
    Audio(AudioBranch),
}

impl Branch {

    pub fn new(stream: u32, channel: u32, pid: u32, typ: &str) -> Option<Branch> {
        match typ {
            "video" => Some(Branch::Video(VideoBranch::new(stream, channel, pid))),
           // "audio" => Some(Branch::Audio(AudioBranch::new(stream, channel, pid))),
            _       => None
        }
    }
    
    pub fn plug(&self, src_pad: &gst::Pad) {
        match *self {
            Branch::Video(ref br) => br.plug(src_pad),
            Branch::Audio(ref br) => br.plug(src_pad),
        };
    }

    pub fn add_to_pipe (&self, b: &gst::Bin) {
        match *self {
            Branch::Video(ref br) => br.add_to_pipe(b),
            Branch::Audio(ref br) => br.add_to_pipe(b),
        }
    }    
}