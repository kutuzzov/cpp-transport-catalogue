#include "serialization.h"

#include <fstream>

namespace serialization {

void Serialize(const transport::Catalogue& db, const renderer::MapRenderer& renderer, std::ostream& out) {
	proto_transport::TransportCatalogue proto_db;

	SerializeStops(db, proto_db);
	SerializeStopDistances(db, proto_db);
	SerializeBuses(db, proto_db);
    SerializeRenderSettings(renderer, proto_db);
    //SerializeRouterSettings(router, proto_db);
	
	proto_db.SerializeToOstream(&out);
}

std::tuple<transport::Catalogue, renderer::MapRenderer, transport::Router> Deserialize(std::istream& input) {
	proto_transport::TransportCatalogue proto_db;
	proto_db.ParseFromIstream(&input);

	transport::Catalogue db;

	DeserializeStops(db, proto_db);
	DeserializeStopDistances(db, proto_db);
	DeserializeBuses(db, proto_db);
    
    renderer::RenderSettings render_settings;
    renderer::MapRenderer renderer = DeserializeRenderSettings(render_settings, proto_db);
    transport::Router router{}; // = DeserializeRouterSettings(proto_db);

    return { std::move(db), std::move(renderer), std::move(router) };
}

void SerializeStops(const transport::Catalogue& db, proto_transport::TransportCatalogue& proto_db) {
	const auto all_stops = db.GetSortedAllStops();
	for (const auto& stop : all_stops) {
		proto_transport::Stop proto_stop;
		proto_stop.set_name(stop.second->name);
		proto_stop.mutable_coordinates()->set_lat(stop.second->coordinates.lat);
		proto_stop.mutable_coordinates()->set_lng(stop.second->coordinates.lng);
		for (const auto& bus : stop.second->buses_by_stop) {
			proto_stop.add_buses_by_stop(bus);
		}
		*proto_db.add_stops() = std::move(proto_stop);
	}
}

void SerializeStopDistances(const transport::Catalogue& db, proto_transport::TransportCatalogue& proto_db) {
	for (const auto& [stop_pair, distance] : db.GetStopDistances()) {
		proto_transport::StopDistanses proto_stop_distances;
		proto_stop_distances.set_from(stop_pair.first->name);
		proto_stop_distances.set_to(stop_pair.second->name);
		proto_stop_distances.set_distance(distance);

		*proto_db.add_stop_distances() = std::move(proto_stop_distances);
	}
}

void SerializeBuses(const transport::Catalogue& db, proto_transport::TransportCatalogue& proto_db) {
	const auto all_buses = db.GetSortedAllBuses();
	for (const auto& bus : all_buses) {
		proto_transport::Bus proto_bus;
		proto_bus.set_number(bus.second->number);
		for (const auto* stop : bus.second->stops) {
			*proto_bus.mutable_stops()->Add() = stop->name;
		}
		proto_bus.set_is_circle(bus.second->is_circle);
		*proto_db.add_buses() = std::move(proto_bus);
	}
}

void SerializeRenderSettings(const renderer::MapRenderer& renderer, proto_transport::TransportCatalogue& proto_db) {
    const auto render_settings = renderer.GetRenderSettings();
    proto_map::RenderSettings proto_render_settings;
    proto_render_settings.set_width(render_settings.width);
    proto_render_settings.set_height(render_settings.height);
    proto_render_settings.set_padding(render_settings.padding);
    proto_render_settings.set_stop_radius(render_settings.stop_radius);
    proto_render_settings.set_line_width(render_settings.line_width);
    proto_render_settings.set_bus_label_font_size(render_settings.bus_label_font_size);
    *proto_render_settings.mutable_bus_label_offset() = SerializePoint(render_settings.bus_label_offset);
    proto_render_settings.set_stop_label_font_size(render_settings.stop_label_font_size);
    *proto_render_settings.mutable_stop_label_offset() = SerializePoint(render_settings.stop_label_offset);
    *proto_render_settings.mutable_underlayer_color() = SerializeColor(render_settings.underlayer_color);
    proto_render_settings.set_underlayer_width(render_settings.underlayer_width);
    for (const auto& color : render_settings.color_palette) {
        *proto_render_settings.add_color_palette() = SerializeColor(color);
    }
    *proto_db.mutable_render_settings() = std::move(proto_render_settings);
}

proto_map::Point SerializePoint(const svg::Point& point) {
    proto_map::Point proto_point;
    proto_point.set_x(point.x);
    proto_point.set_y(point.y);
    return proto_point;
}

proto_map::Color SerializeColor(const svg::Color& color) {
    proto_map::Color proto_color;
    switch (color.index()) {
        case 1: {
            proto_color.set_name(std::get<1>(color));
            break;
        }
        case 2: {
            *proto_color.mutable_rgb() = SerializeRgb(std::get<2>(color));
            break;
        }
        case 3: {
            *proto_color.mutable_rgba() = SerializeRgba(std::get<3>(color));
            break;
        }
    }
    return proto_color;
}

proto_map::Rgb SerializeRgb(const svg::Rgb& rgb) {
    proto_map::Rgb proto_rgb;
    proto_rgb.set_red(rgb.red);
    proto_rgb.set_blue(rgb.blue);
    proto_rgb.set_green(rgb.green);
    
    return proto_rgb;
}

proto_map::Rgba SerializeRgba(const svg::Rgba& rgba) {
    proto_map::Rgba proto_rgba;
    proto_rgba.set_red(rgba.red);
    proto_rgba.set_blue(rgba.blue);
    proto_rgba.set_green(rgba.green);
    proto_rgba.set_opacity(rgba.opacity);
    
    return proto_rgba;
}

void SerializeRouterSettings(const transport::Router& router, proto_transport::TransportCatalogue& proto_db) {
    proto_transport::RouterSettings proto_router_settings;
    proto_router_settings.set_bus_wait_time(router.GetBusWaitTime());
    proto_router_settings.set_bus_velocity(router.GetBusVelocity());
    *proto_db.mutable_router_settings() = std::move(proto_router_settings);
}

void DeserializeStops(transport::Catalogue& db, const proto_transport::TransportCatalogue& proto_db) {
	for (int i = 0; i < proto_db.stops_size(); ++i) {
		const proto_transport::Stop& proto_stop = proto_db.stops(i);
		db.AddStop(proto_stop.name(), { proto_stop.coordinates().lat(), proto_stop.coordinates().lng() });
	}
}

void DeserializeStopDistances(transport::Catalogue& db, const proto_transport::TransportCatalogue& proto_db) {
	for (int i = 0; i < proto_db.stop_distances_size(); ++i) {
		const proto_transport::StopDistanses& proto_stop_distances = proto_db.stop_distances(i);
		const transport::Stop* from = db.FindStop(proto_stop_distances.from());
		const transport::Stop* to = db.FindStop(proto_stop_distances.to());
		db.SetDistance(from, to, proto_stop_distances.distance());
	}
}

void DeserializeBuses(transport::Catalogue& db, const proto_transport::TransportCatalogue& proto_db) {
	for (int i = 0; i < proto_db.buses_size(); ++i) {
		const proto_transport::Bus& proto_bus = proto_db.buses(i);
		std::vector<const transport::Stop*> stops(proto_bus.stops_size());
		for (int j = 0; j < stops.size(); ++j) {
			stops[j] = db.FindStop(proto_bus.stops(j));
		}
		db.AddRoute(proto_bus.number(), stops, proto_bus.is_circle());
	}
}

renderer::MapRenderer DeserializeRenderSettings(renderer::RenderSettings& render_settings, const proto_transport::TransportCatalogue& proto_db) {
    const proto_map::RenderSettings& proto_render_settings = proto_db.render_settings();
    render_settings.width = proto_render_settings.width();
    render_settings.height = proto_render_settings.height();
    render_settings.padding = proto_render_settings.padding();
    render_settings.stop_radius = proto_render_settings.stop_radius();
    render_settings.line_width = proto_render_settings.line_width();
    render_settings.bus_label_font_size = proto_render_settings.bus_label_font_size();
    render_settings.bus_label_offset = DeserializePoint(proto_render_settings.bus_label_offset());
    render_settings.stop_label_font_size = proto_render_settings.stop_label_font_size();
    render_settings.stop_label_offset = DeserializePoint(proto_render_settings.stop_label_offset());
    render_settings.underlayer_color = DeserializeColor(proto_render_settings.underlayer_color());
    render_settings.underlayer_width = proto_render_settings.underlayer_width();
    for (int i = 0; i < proto_render_settings.color_palette_size(); ++i) {
        render_settings.color_palette.push_back(DeserializeColor(proto_render_settings.color_palette(i)));
    }
    return render_settings;
}

svg::Point DeserializePoint(const proto_map::Point& proto_point) {
    return { proto_point.x(), proto_point.y() };
}

svg::Color DeserializeColor(const proto_map::Color& proto_color) {
    if (proto_color.has_rgb()) return svg::Rgb{ static_cast<uint8_t>(proto_color.rgb().red()),
                                                static_cast<uint8_t>(proto_color.rgb().green()),
                                                static_cast<uint8_t>(proto_color.rgb().blue()) };
    else if (proto_color.has_rgba()) return svg::Rgba{ static_cast<uint8_t>(proto_color.rgba().red()),
                                                        static_cast<uint8_t>(proto_color.rgba().green()),
                                                        static_cast<uint8_t>(proto_color.rgba().blue()),
                                                        proto_color.rgba().opacity() };
    else return proto_color.name();
    
    throw std::runtime_error("Error deserialized color");
}

transport::Router DeserializeRouterSettings(const proto_transport::TransportCatalogue& proto_db) {
    return { proto_db.router_settings().bus_wait_time(), proto_db.router_settings().bus_velocity() };
}

} // serialization
