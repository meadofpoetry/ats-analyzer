use std::collections::LinkedList;

pub struct Signal<T> {
    callbacks: LinkedList<Box<Fn(&T) + Send + Sync + 'static>>
}

impl<T> Signal<T> {
    pub fn new() -> Signal<T> {
        Signal { callbacks: LinkedList::new() }
    }

    pub fn connect<F>(&mut self, f: F)
        where F: Fn(&T) + Send + Sync + 'static {
        self.callbacks.push_front(Box::new(f))
    }

    pub fn emit(&self, v: &T) {
        for f in self.callbacks.iter() {
            f(v)
        }
    }

    pub fn disconnect_all(&mut self) {
        self.callbacks.clear()
    }
}

pub struct Msg<T,R> {
    callback: Option<Box<Fn(T)->R + Send + Sync + 'static>>
}

impl<T,R> Msg<T,R> {
    pub fn new() -> Msg<T,R> {
        Msg { callback: None }
    }

    pub fn connect<F>(&mut self, f: F) -> Result<(),String>
        where F: Fn(T)->R + Send + Sync + 'static {
        match self.callback {
            Some(_) => Err(String::from("Connected")),
            None    => Ok(self.callback = Some(Box::new(f)))
        }
    }

    pub fn emit(&self, v: T) -> Option<R> {
        match self.callback {
            Some (ref f) => Some (f(v)),
            None         => None
        }
    }

    pub fn disconnect(&mut self) {
        self.callback = None;
    }
}

