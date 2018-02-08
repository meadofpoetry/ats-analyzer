use gst::prelude::*;
use gst;
use std::sync::{Arc,Mutex};
use metadata::{Channel,Structure};
use signals::Signal;
use pad::SrcPad;
use branch::Branch;
use std::u32;

pub struct Root {
    bin:           gst::Bin,
    src:           gst::Element,
    tee:           gst::Element,
    branches:      Arc<Mutex<Vec<Branch>>>,
    pub pad_added: Arc<Mutex<Signal<SrcPad>>>,
}

impl Root {

    fn build_branch (branches: &mut Vec<Branch>,
                     stream: u32, chan: &Channel,
                     added: Arc<Mutex<Signal<SrcPad>>>,
                     bin: gst::Bin, pad: &gst::Pad) {
        let pname = pad.get_name();
        let pcaps = String::from(pad.get_current_caps().unwrap().get_structure(0).unwrap().get_name());
        let name_toks: Vec<&str> = pname.split('_').collect();
        let caps_toks: Vec<&str> = pcaps.split('/').collect();
        
        if name_toks.len() != 3 || caps_toks.len() == 0 { return };
        let typ = caps_toks[0];
        let pid = u32::from_str_radix(name_toks[2], 16).unwrap();
        if let Some (p) = chan.find_pid(pid){
            if !p.to_be_analyzed { return };
        };

        if let Some(branch) = Branch::new(stream, chan.number, pid, typ) {
            branch.add_to_pipe(&bin);
            branch.plug(&pad);
            match branch {
                Branch::Video(ref b) => b.pad_added.lock().unwrap().connect(move |p| added.lock().unwrap().emit(p)),
                Branch::Audio(ref b) => b.pad_added.lock().unwrap().connect(move |p| added.lock().unwrap().emit(p)),
            };

            branches.push(branch);
            bin.set_state(gst::State::Playing);
        }
    }
    
    pub fn new(bin: gst::Bin, m: Structure) -> Option<Root> {
        if ! m.to_be_analyzed() { return None };

        let src = gst::ElementFactory::make("udpsrc", None).unwrap();
        let tee = gst::ElementFactory::make("tee", None).unwrap();
        let branches = Arc::new(Mutex::new(Vec::new()));
        let pad_added = Arc::new(Mutex::new(Signal::new()));
        
        src.set_property("uri", &m.uri).unwrap();
        src.set_property("buffer-size", &2147483647).unwrap();

        bin.add_many(&[&src, &tee]).unwrap();
        src.link(&tee).unwrap();

        let bin_c = bin.clone();
       // let branches_c = branches.clone();

        for chan in m.channels {
            let demux_name = format!("demux_{}_{}", m.id, chan.number);

            let queue = gst::ElementFactory::make("queue", None).unwrap();
            let demux = gst::ElementFactory::make("tsdemux", Some(demux_name.as_str())).unwrap();

            demux.set_property("program-number", &(chan.number as i32)).unwrap();
            queue.set_property("max-size-buffers", &200000u32).unwrap();
            queue.set_property("max-size-bytes", &429496729u32).unwrap();

            let sinkpad = queue.get_static_pad("sink").unwrap();
            let srcpad  = tee.get_request_pad("src_%u").unwrap();

            bin_c.add_many(&[&queue,&demux]).unwrap();
            queue.link(&demux).unwrap();

            srcpad.link(&sinkpad);
            let stream = m.id as u32;

            let bin_cc = bin_c.clone();
            let branches_c = branches.clone();
            let pad_added_c = pad_added.clone();

            demux.connect_pad_added(move | _, pad | {
                Root::build_branch(&mut branches_c.lock().unwrap(), stream, &chan,
                                   pad_added_c.clone(), bin_cc.clone(), pad);
            });
        };

        Some(Root { bin, src, tee, branches, pad_added })
    }
}