//
// Copyright (c) 2017, 2020 ADLINK Technology Inc.
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   ADLINK zenoh team, <zenoh@adlink-labs.tech>
//

use clap::{Arg, ArgMatches};
use zenoh::{
    net::{runtime::Runtime, Session},
    ZFuture,
};
use zenoh_plugin_trait::prelude::*;

use zenohc::net::zn_session_t;
pub struct CPlugin;
pub struct NoCPlugins;
impl std::error::Error for NoCPlugins {}
impl std::fmt::Debug for NoCPlugins {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::result::Result<(), std::fmt::Error> {
        write!(f, "NoCPlugins")
    }
}
impl std::fmt::Display for NoCPlugins {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "No C plugins requested")
    }
}
zenoh_plugin_trait::declare_plugin!(CPlugin);
impl Plugin for CPlugin {
    type Requirements = Vec<Arg<'static, 'static>>;

    type StartArgs = (Runtime, ArgMatches<'static>);

    fn compatibility() -> zenoh_plugin_trait::PluginId {
        zenoh_plugin_trait::PluginId { uid: "cplugins" }
    }

    fn get_requirements() -> Self::Requirements {
        vec![Arg::from_usage(
            "--c-plugin=[DLIBPATH] \
        'The path to a C plugin for zenoh. Repeat this option to run several plugins.'",
        )]
    }

    fn start(
        (runtime, args): &Self::StartArgs,
    ) -> Result<zenoh_plugin_trait::BoxedAny, Box<dyn std::error::Error>> {
        let plugins = match args.values_of("c-plugin") {
            Some(plugins) => plugins,
            None => return Err(Box::new(NoCPlugins)),
        };
        for plugin in plugins {
            unsafe {
                let plugin = libloading::Library::new(plugin)?;
                let start = plugin.get::<extern "C" fn(*mut zn_session_t)>(b"start_plugin")?;
                let session = Box::new(zn_session_t::from(
                    Session::init(runtime.clone(), true, vec![], vec![]).wait(),
                ));
                start(Box::into_raw(session));
            }
        }
        Ok(Box::new(()))
    }
}
