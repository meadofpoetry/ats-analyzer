type error = [ `Qoe_backend of string ]

module Make
         (Id : Qoe_backend_types.Basic.STREAM_ID)
         (Uri_string : Qoe_backend_types.Basic.URI)
         (Useconds : Qoe_backend_types.Basic.USECONDS)
  = struct

  module Structure = Qoe_backend_types.Structure.Make (Id) (Uri_string)

  module Wm = Qoe_backend_types.Wm.Make (Id)

  module Qoe_errors = Qoe_backend_types.Qoe_errors.Make (Id) (Useconds)

  module Qoe_status = Qoe_backend_types.Qoe_status.Make (Id)

  module Qoe_error_parser = Qoe_errors_parser.Make (Id) (Useconds)
  
  type t = Qoe_backend.t

  type events = { streams : Structure.t list React.event
                ; graph : Structure.t list React.event
                ; wm : Wm.t React.event
                ; vdata : Qoe_errors.Video_data.t React.event
                ; adata : Qoe_errors.Audio_data.t React.event
                }
              
  let init_logger = Qoe_backend.init_logger

  let make_event of_string =
    let e, push = Lwt_react.E.create () in
    let data = ref None in
    let notif = Lwt_unix.make_notification
                  (fun () -> match !data with
                             | None -> ()
                             | Some data -> push data)
    in
    let cb s =
      try data := Some (of_string s);
          Lwt_unix.send_notification notif
      with _ -> ()
    in
    cb, e

  let make_video_event ()  =
    let open Qoe_backend in
    let open Qoe_errors.Video_data in
    let e, push = Lwt_react.E.create () in
    let data = ref None in
    let notif = Lwt_unix.make_notification
                  (fun () -> match !data with
                             | None -> ()
                             | Some data -> push data)
    in
    let cb id channel pid buf =
      try let errors = Gstbuffer.process_unsafe buf Qoe_error_parser.get_video_errors in 
          data := Some { stream = Id.of_string id
                       ; channel
                       ; pid
                       ; errors
                    };
          Lwt_unix.send_notification notif
      with _ -> ()
    in cb, e

  let make_audio_event ()  =
    let open Qoe_backend in
    let open Qoe_errors.Audio_data in
    let e, push = Lwt_react.E.create () in
    let data = ref None in
    let notif = Lwt_unix.make_notification
                  (fun () -> match !data with
                             | None -> ()
                             | Some data -> push data)
    in
    let cb id channel pid buf =
      try let errors = Gstbuffer.process_unsafe buf Qoe_error_parser.get_audio_errors in 
          data := Some { stream = Id.of_string id
                       ; channel
                       ; pid
                       ; errors
                    };
          Lwt_unix.send_notification notif
      with _ -> ()
    in cb, e

  let of_json conv js =
    Yojson.Safe.from_string js
    |> conv
    |> function Ok v -> v | Error e -> failwith e

  let of_json_err conv js =
    Yojson.Safe.from_string js
    |> conv
    |> function Ok _ as v -> v
              | Error e -> Error (`Qoe_backend e)

  let to_json conv v =
    conv v
    |> Yojson.Safe.to_string
                         
  let get conv f x =
    Lwt_preemptive.detach (fun obj ->
        try f obj
            |> of_json_err conv
        with Failure e -> Error (`Qoe_backend e))
      x

  let set conv f data x =
    Lwt_preemptive.detach (fun obj ->
        try Ok (f obj (to_json conv data))
        with Failure e -> Error (`Qoe_backend e))
      x

  (* TODO change args type to Id * URI *)
  let create args =
    try
      let streams_cb, streams = make_event (of_json Structure.many_of_yojson) in
      let graph_cb, graph = make_event (of_json Structure.many_of_yojson) in
      let wm_cb, wm = make_event (of_json Wm.of_yojson) in
      let vdata_cb, vdata = make_video_event () in
      let adata_cb, adata = make_audio_event () in
      let backend = Qoe_backend.create args
                      ~streams:streams_cb
                      ~graph:graph_cb
                      ~wm:wm_cb
                      ~vdata:vdata_cb
                      ~adata:adata_cb
      and events = { streams
                   ; graph
                   ; wm
                   ; vdata
                   ; adata
                   }
      in Lwt.return_ok (backend, events)
    with Failure e -> Lwt.return_error (`Qoe_backend e)

  let run backend =
    Lwt_preemptive.detach Qoe_backend.run backend

  let destroy backend =
    Qoe_backend.free backend

  module Stream_parser = struct

    let get_structure backend =
      get
        Structure.of_yojson
        Qoe_backend.stream_parser_get_structure
        backend

  end

  module Graph = struct

    let get_structure backend =
      get
        Structure.of_yojson
        Qoe_backend.graph_get_structure
        backend
      
    let apply_structure backend data =
      set
        Structure.to_yojson
        Qoe_backend.graph_apply_structure
        data
        backend

  end

  module Mosaic = struct

    let get_layout backend =
      get
        Wm.of_yojson
        Qoe_backend.wm_get_layout
        backend
      
    let apply_layout backend data =
      set
        Wm.to_yojson
        Qoe_backend.wm_apply_layout
        data
        backend

  end
               
end
