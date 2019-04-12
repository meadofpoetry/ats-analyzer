//use chatterer::ChattererProxy;
//use chatterer::Logger;
//use settings::Configuration;
use probe::Probe;
use channels;
//use control::Control;
use streams::StreamParser;
use metadata::Structure;
use wm::template::WmTemplatePartial;
use graph::Graph;
use gst;
use glib;
use std::sync::{Arc,Mutex};

pub struct ContextState {
    stream_parser: StreamParser,
    graph:         Graph,
    probes:        Vec<Probe>,
}

pub struct Context {
    pub state:   Arc<Mutex<ContextState>>,
    //control:     Control,
    mainloop:    glib::MainLoop,
}

impl ContextState {

    pub fn stream_parser_get_structure (&self) -> Vec<u8> {
        println!("Getting streams");
        let s : &Vec<_> = &self.stream_parser.structures.lock().unwrap();
        serde_json::to_vec(&s).unwrap()
    }

    pub fn graph_get_structure (&self) -> Vec<u8> {
        let s = self.graph.get_structure();
        serde_json::to_vec(&s).unwrap()
    }

    pub fn graph_apply_structure (&self, v: &[u8]) -> Result<(),String> {
        if let Ok (structs) = serde_json::from_slice::<Vec<Structure>>(&v) {
            self.graph.set_structure(structs)
        } else {
            Err (String::from("msg format err"))
        }
    }

    pub fn wm_get_layout (&self) -> Result<Vec<u8>,String> {
        match self.graph.get_wm_layout () {
            Ok(s)  => Ok(serde_json::to_vec(&s).unwrap()),
            Err(e) => Err(e),
        }
    }

    pub fn wm_apply_layout (&self, v: &[u8]) -> Result<(),String> {
        if let Ok(templ) = serde_json::from_slice::<WmTemplatePartial>(&v) {
            self.graph.set_wm_layout(templ)
        } else {
            Err (String::from("msg format err"))
        }
    }
    
}

impl Context {
    pub fn new (uris : &Vec<(String,String)>,
                streams_cb: channels::Callbacks<Vec<u8>>,
                graph_cb: channels::Callbacks<Vec<u8>>,
                wm_cb: channels::Callbacks<Vec<u8>>,
    ) -> Result<Box<Context>, String> {
        
        gst::init().unwrap();

        let mainloop = glib::MainLoop::new(None, false);
        //let control  = Control::new().unwrap();
        
        let mut probes  = Vec::new();

        for sid in 0..uris.len() {
            probes.push(Probe::new(&uris[sid]));
        };

        let stream_sender = channels::create (streams_cb);
        let graph_sender = channels::create (graph_cb);
        let wm_sender = channels::create (wm_cb);

        //let     config        = Configuration::new(i.msg_type, control.sender.clone());
        let mut stream_parser = StreamParser::new(stream_sender);
        let     graph         = Graph::new(graph_sender, wm_sender).unwrap();
        
        for probe in &mut probes {
            probe.set_state(gst::State::Playing);
            stream_parser.connect_probe(probe);
        }
        
        //let wm = graph.get_wm();

        /*
        let dis = dispatcher.clone();
        control.connect(move |s| {
            // debug!("Control message received");
            dis.lock().unwrap().dispatch(s).unwrap()
            // debug!("Control message response ready");
        });
        */

        //graph.connect_destructive(&mut stream_parser.update.lock().unwrap());
        //graph.connect_settings(&mut config.update.lock().unwrap());

        let state = Arc::new (Mutex::new (ContextState { stream_parser, graph, probes }));
        
        info!("Context was created");
        Ok(Box::new(Context { state, mainloop }))
    }

    pub fn run (&mut self) {
    //    let context_state = self.state.clone();
/*
        self.control.connect(move |s| {
            context_state.lock().unwrap().dispatch(&s)
        });
  */      
        self.mainloop.run();
    }

    pub fn quit (&mut self) {
        self.mainloop.quit()
    }
    
}
