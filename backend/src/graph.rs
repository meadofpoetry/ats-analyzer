use gst::prelude::*;
use gst;
use std::sync::{Arc,Mutex};
use std::sync::mpsc::Sender;
use chatterer::MsgType;
use chatterer::notif::Notifier;
use chatterer::control::{Addressable,Replybox};
use root::Root;
use wm::Wm;
use signals::Msg;
use metadata::Structure;
use settings::Settings;
use audio_mux::Mux;
use renderer::{VideoR,AudioR,Renderer};

pub struct GraphState {
    format:      MsgType,
    sender:      Sender<Vec<u8>>,
    settings:    Option<Settings>,
    structure:   Option<Vec<Structure>>,

    pipeline:    gst::Pipeline,
    bus:         gst::Bus,
    pub wm:      Arc<Mutex<Wm>>,
    pub mux:     Arc<Mutex<Mux>>,
    roots:       Vec<Root>,
    vrend:       Option<Renderer<VideoR>>,
    arend:       Option<Renderer<AudioR>>,
}

pub struct Graph {
    format:      MsgType,
    pub chat:    Arc<Mutex<Notifier>>,

    state:       Arc<Mutex<GraphState>>,
}

impl Addressable for Graph {
    fn get_name(&self) -> &str { "graph" }
    fn get_format(&self) -> MsgType { self.format }
}

impl Replybox<(),Option<Vec<Structure>>> for Graph {
    fn reply (&self) -> Box<Fn(())->Result<Option<Vec<Structure>>,String> + Send + Sync> {
        let state = self.state.clone();
        
        Box::new(move |()| {
            if let Ok (s) = state.lock() {
                Ok (s.structure.clone())
            } else {
                Err (String::from("can't acquire graph state applied structure"))
            }
        })
    }
}

impl GraphState {
    pub fn new (format: MsgType, sender: Sender<Vec<u8>>) -> GraphState {
        let settings  = None;
        let structure = None;
        let pipeline  = gst::Pipeline::new(None);
        let wm        = Arc::new(Mutex::new(Wm::new(format, sender.clone())));
        let mux       = Arc::new(Mutex::new(Mux::new(format, sender.clone())));
        let vrend     = None;
        let arend     = None;
        let bus       = pipeline.get_bus().unwrap();
        let roots     = Vec::new();
        wm.lock().unwrap().init(&pipeline);
        mux.lock().unwrap().init(&pipeline);
        GraphState { settings, structure, pipeline, wm, mux, bus, roots, arend, vrend, format, sender }
    }

    pub fn reset (&mut self) {
        debug!("GraphState::reset [pipeline pause]");
        gst::debug_bin_to_dot_file(&self.pipeline, gst::DebugGraphDetails::VERBOSE, "pipeline_pre_reset");
        let _ = self.pipeline.set_state(gst::State::Null);
        debug!("GraphState::reset [pipeline refcounter] {}", self.pipeline.ref_count());
        
        debug!("GraphState::reset [pipeline reset]");
        self.pipeline = gst::Pipeline::new(None);
        debug!("GraphState::reset [bus reset]");
        self.bus      = self.pipeline.get_bus().unwrap();
        debug!("GraphState::reset [root reset]");
        self.roots    = Vec::new();       
        debug!("GraphState::reset [vrend reset]");
        self.vrend    = None;
        debug!("GraphState::reset [arend reset]");
        self.arend    = None;
        debug!("GraphState::reset [audio mux reset]");
        self.mux.lock().unwrap().reset();
        debug!("GraphState::reset [wm reset]");
        self.wm.lock().unwrap().reset();
        gst::debug_bin_to_dot_file(& self.pipeline, gst::DebugGraphDetails::VERBOSE, "pipeline_post_reset");
    }

    pub fn init (&mut self) {
        debug!("GraphState::init");
        self.wm.lock().unwrap().init(&self.pipeline);
        self.mux.lock().unwrap().init(&self.pipeline);
        self.vrend    = Some(Renderer::<VideoR>::new(5004, &self.pipeline));
        self.arend    = Some(Renderer::<AudioR>::new(5005, &self.pipeline));
        debug!("GraphState::reset [video render reset]");
        self.vrend.iter().for_each(|rend| rend.plug(&self.wm.lock().unwrap().src_pad()));
        debug!("GraphState::reset [audio render reset]");
        self.arend.iter().for_each(|rend| rend.plug(&self.mux.lock().unwrap().src_pad()));
    }

    pub fn set_state (&self, st: gst::State) {
        // TODO check
        debug!("GraphState::set_state [{:?}]", st);
        let _ = self.pipeline.set_state(st);
    }

    pub fn apply_streams (&mut self, s: &[Structure]) -> Result<(),String> {
        debug!("Graph::apply_streams [reset]");
        self.reset();
        self.init();
        debug!("Graph::apply_streams [loop]");

        for s in s {
            if let Some(root) = Root::new(&self.pipeline, s.clone(),
                                          self.settings, self.format,
                                          self.sender.clone()) {
                //let pipe   = self.pipeline.clone();
                let wm     = self.wm.clone();
                let mux    = self.mux.clone();
                root.pad_added.lock().unwrap().connect(move |p| {
                    debug!("Graph::apply_streams [attach pad {} {}]", p.stream, p.channel);
                    wm.lock().unwrap().plug(p);
                    // TODO check
                    //let _ = pipe.set_state(gst::State::Playing);
                    //gst::debug_bin_to_dot_file(&pipe, gst::DebugGraphDetails::VERBOSE, "pipeline");
                });
                
                root.audio_pad_added.lock().unwrap().connect(move |p| {
                    mux.lock().unwrap().plug(p);
                });
            }
        };
        // TODO replace with retain_state
        let _ = self.pipeline.set_state(gst::State::Playing);
        self.structure = Some(Vec::from(s));
        Ok(())
    }

    pub fn apply_settings (&mut self, s: Settings) -> Result<(),String> {
        debug!("Graph::apply_settings");
        self.settings = Some(s);
        self.roots.iter_mut().for_each(|root : &mut Root| root.apply_settings(s) );
        Ok(())
    }
}

impl Graph {
    
    pub fn new (format: MsgType, sender: Sender<Vec<u8>>) -> Result<Graph,String> {
        let chat = Arc::new(Mutex::new( Notifier::new("graph", format, sender.clone() )));
        let state = Arc::new(Mutex::new(GraphState::new(format, sender.clone()) ));
        Ok(Graph { format, chat, state } )
    }

    pub fn get_wm (&self) -> Arc<Mutex<Wm>> {
        self.state.lock().unwrap().wm.clone()
    }
    
    pub fn connect_destructive (&mut self, msg: &mut Msg<Vec<Structure>,Result<(),String>>) {
        let state  = self.state.clone();
        let notif  = self.chat.clone();
        msg.connect(move |s| {
            debug!("Graph::destructive");
            let res = state.lock().unwrap().apply_streams(&s);
            if res.is_ok() {
                debug!("Graph::destructive [send state]");
                notif.lock().unwrap().talk(&s)
            };
            res
        }).unwrap();
    }

    pub fn connect_settings (&mut self, msg: &mut Msg<Settings,Result<(),String>>) {
        let state  = self.state.clone();
        msg.connect(move |s| {
            debug!("Graph::settings");
            state.lock().unwrap().apply_settings(s)
        }).unwrap();
    }
    
}
