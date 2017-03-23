#include "settings.hpp"
#include "json.hpp"

#include <cstdio>
#include <algorithm>
#include <iostream>

using namespace std;
using namespace Ats;

/* ---------- Channel settings ----------- */

string
Settings::Channel_settings::Error_overlay::to_json() const {
    constexpr int size = 1024;

    char buffer[size];
    constexpr const char* fmt = "{"
        "\"error_color\":%d,"
        "\"enabled\":%s"
        "}";

    int n = snprintf (buffer, size, fmt, error_color, Ats::to_string(enabled).c_str());
    if ( (n>=0) && (n<size) ) return string(buffer);
    else throw Serializer_failure ();
}

string
Settings::Channel_settings::Channel_name::to_json() const {
    constexpr int size = 1024;

    char buffer[size];
    constexpr const char* fmt = "{"
        "\"enabled\":%s,"
        "\"font_size\":%d,"
        "\"fmt\":\"%s\""
        "}";

    int n = snprintf (buffer, size, fmt, Ats::to_string(enabled).c_str(), font_size, this->fmt.c_str());
    if ( (n>=0) && (n<size) ) return string(buffer);
    else throw Serializer_failure ();
}

string
Settings::Channel_settings::Audio_meter::to_json() const {
    constexpr int size = 1024;

    char buffer[size];
    constexpr const char* fmt = "{"
        "\"position\":\"%s\","
        "\"width\":%d,"
        "\"height\":%d,"
        "\"peak_color\":\"%s\","
        "\"high_color\":\"%s\","
        "\"mid_color\":\"%s\","
        "\"low_color\":\"%s\","
        "\"background_color\":\"%s\""
        "}";

    int n = snprintf (buffer, size, fmt,
                      position == Audio_meter_pos::Left ? "left" :
                      position == Audio_meter_pos::Right ? "right" :
                      "off",
                      width, height,
                      peak_color.c_str(),
                      high_color.c_str(),
                      mid_color.c_str(),
                      low_color.c_str(),
                      background_color.c_str());
    if ( (n>=0) && (n<size) ) return string(buffer);
    else throw Serializer_failure ();
}

string
Settings::Channel_settings::Status_bar::to_json() const {
    constexpr int size = 1024;

    char buffer[size];
    constexpr const char* fmt = "{"
        "\"position\":\"%s\","
        "\"aspect\":%s,"
        "\"subtitles\":%s,"
        "\"teletext\":%s,"
        "\"eit\":%s,"
        "\"qos\":%s,"
        "\"scte35\":%s"
        "}";

    int n = snprintf (buffer, size, fmt,
                      position == Status_bar_pos::Top_left ? "top_left" :
                      position == Status_bar_pos::Top_right ? "top_right" :
                      position == Status_bar_pos::Left ? "left" :
                      position == Status_bar_pos::Right ? "right" :
                      position == Status_bar_pos::Bottom_left ? "bottom_left" :
                      position == Status_bar_pos::Bottom_right ? "bottom_right" :
                      "off",
                      Ats::to_string(aspect).c_str(),
                      Ats::to_string(subtitles).c_str(),
                      Ats::to_string(teletext).c_str(),
                      Ats::to_string(eit).c_str(),
                      Ats::to_string(qos).c_str(),
                      Ats::to_string(scte35).c_str());
    if ( (n>=0) && (n<size) ) return string(buffer);
    else throw Serializer_failure ();
}

string
Settings::Channel_settings::to_json() const {
    constexpr int size = 1024;

    char buffer[size];
    constexpr const char* fmt = "{"
        "\"error_overlay\":%s,"
        "\"channel_name\":%s,"
        "\"audio_meter\":%s,"
        "\"status_bar\":%s,"
        "\"show_border\":%s,"
        "\"border_color\":\"%s\","
        "\"show_aspect_border\":%s,"
        "\"aspect_border_color\":\"%s\""
        "}";

    int n = snprintf (buffer, size, fmt,
                      error_overlay.to_json().c_str(),
                      channel_name.to_json().c_str(),
                      audio_meter.to_json().c_str(),
                      status_bar.to_json().c_str(),
                      Ats::to_string(show_border).c_str(),
                      border_color.c_str(),
                      Ats::to_string(show_aspect_border).c_str(),
                      aspect_border_color.c_str());
    if ( (n>=0) && (n<size) ) return string(buffer);
    else throw Serializer_failure ();
}

/* ---------- QoE settings --------------- */

string
Settings::Qoe_settings::to_json() const {
    constexpr int size = 1024;

    char buffer[size];
    constexpr const char* fmt = "{"
        "\"vloss\":%.2f,"
        "\"aloss\":%.2f,"
        "\"black_cont_en\":%s,"
        "\"black_cont\":%.2f,"
        "\"black_peak_en\":%s,"
        "\"black_peak\":%.2f,"
        "\"luma_cont_en\":%s,"
        "\"luma_cont\":%.2f,"
        "\"luma_peak_en\":%s,"
        "\"luma_peak\":%.2f,"
        "\"black_time\":%.2f,"
        "\"black_pixel\":%d,"
        "\"freeze_cont_en\":%s,"
        "\"freeze_cont\":%.2f,"
        "\"freeze_peak_en\":%s,"
        "\"freeze_peak\":%.2f,"
        "\"diff_cont_en\":%s,"
        "\"diff_cont\":%.2f,"
        "\"diff_peak_en\":%s,"
        "\"diff_peak\":%.2f,"
        "\"freeze_time\":%.2f,"
        "\"pixel_diff\":%d,"
        "\"blocky_cont_en\":%s,"
        "\"blocky_cont\":%.2f,"
        "\"blocky_peak_en\":%s,"
        "\"blocky_peak\":%.2f,"
        "\"blocky_time\":%.2f,"
        "\"mark_blocks\":%s,"
        "\"silence_cont_en\":%s,"
        "\"silence_cont\":%.2f,"
        "\"silence_peak_en\":%s,"
        "\"silence_peak\":%.2f,"
        "\"silence_time\":%.2f,"
        "\"loudness_cont_en\":%s,"
        "\"loudness_cont\":%.2f,"
        "\"loudness_peak_en\":%s,"
        "\"loudness_peak\":%.2f,"
        "\"loudness_time\":%.2f,"
        "\"adv_diff\":%.2f,"
        "\"adv_buff\":%d"
        "}";

    int n = snprintf (buffer, size, fmt,
                      vloss, aloss,
                      Ats::to_string(black_cont_en).c_str(), black_cont,
                      Ats::to_string(black_peak_en).c_str(), black_peak,
                      Ats::to_string(luma_cont_en).c_str(), luma_cont,
                      Ats::to_string(luma_peak_en).c_str(), luma_peak,
                      black_time, black_pixel,
                      Ats::to_string(freeze_cont_en).c_str(), freeze_cont,
                      Ats::to_string(freeze_peak_en).c_str(), freeze_peak,
                      Ats::to_string(diff_cont_en).c_str(), diff_cont,
                      Ats::to_string(diff_peak_en).c_str(), diff_peak,
                      freeze_time, pixel_diff,
                      Ats::to_string(blocky_cont_en).c_str(), blocky_cont,
                      Ats::to_string(blocky_peak_en).c_str(), blocky_peak,
                      blocky_time, Ats::to_string(mark_blocks).c_str(),
                      Ats::to_string(silence_cont_en).c_str(), silence_cont,
                      Ats::to_string(silence_peak_en).c_str(), silence_peak,
                      silence_time,
                      Ats::to_string(loudness_cont_en).c_str(), loudness_cont,
                      Ats::to_string(loudness_peak_en).c_str(), loudness_peak,
                      loudness_time,
                      adv_diff, adv_buf);
    if ( (n>=0) && (n<size) ) return string(buffer);
    else throw Serializer_failure ();
}

/* ---------- Output sink settings --------------- */

string
Settings::Output_sink::to_json() const {
    constexpr int size = 256;

    char buffer[size];
    constexpr const char* fmt = "{"
        "\"address\":\"%s\","
        "\"port\":%d,"
        "\"enabled\":%s"
        "}";

    int n = snprintf (buffer, size, fmt, address.c_str(), port, Ats::to_string(enabled).c_str());
    if ( (n>=0) && (n<size) ) return string(buffer);
    else throw Serializer_failure ();
}


/* ---------- Settings -------------------- */

// Chatter implementation

string
Settings::to_string() const {
    // string rval = "\tOptions:\n\tStreams:\n";
    // for_each(data.begin(),data.end(),[&rval](const Metadata& m){
    //         rval += "\n";
    //         rval += m.to_string();
    //         rval += "\n";
    //     });
    // rval += "\tOther options:\n";
    // rval += "Dummy: ";
    // rval += std::to_string(resolution.first);
    // rval += "\n";

    string rval = "Settings testing:\n";
    rval += this->to_json();
    rval += "\n";
    return rval;
}

string
Settings::to_json() const {
    constexpr int size = 1024 * 8;

    char buffer[size];
    constexpr const char* fmt = "{"
        "\"qoe_settings\":%s,"
        "\"channel_settings\":%s,"
        "\"output_sink\":%s"
        "}";

    int n = snprintf (buffer, size, fmt,
                      qoe_settings.to_json().c_str(),
                      channel_settings.to_json().c_str(),
                      output_sink_settings.to_json().c_str());
    if ( (n>=0) && (n<size) ) return string(buffer);
    else throw Serializer_failure ();
}

string
Settings::to_msgpack() const {
    return "todo";
}

void
Settings::of_json(const string& j) {
    bool o_set = false;
    
    using json = nlohmann::json;
    auto js = json::parse(j);

    // TODO throw Wrong_json
    if (! js.is_object()) return;

    for (json::iterator el = js.begin(); el != js.end(); ++el) {
        if (el.key() == "option" && el.value().is_string()) {
            o_set = true; // not so serious option
        }
    }

    if (o_set) set.emit(*this);
}

void
Settings::of_msgpack(const string&) {
    set.emit(*this);
}