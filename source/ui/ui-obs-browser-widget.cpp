// AUTOGENERATED COPYRIGHT HEADER START
// Copyright (C) 2023 Michael Fabian 'Xaymar' Dirks <info@xaymar.com>
// AUTOGENERATED COPYRIGHT HEADER END

#include "ui-obs-browser-widget.hpp"
#include "plugin.hpp"

#include "warning-disable.hpp"
#include "../third-party/obs-studio/plugins/obs-browser/panel/browser-panel.hpp"

#include <mutex>
#ifdef D_PLATFORM_LINUX
#include <errno.h>
#include <stdlib.h>
#endif

#include <QGridLayout>
#include <QLabel>
#include "warning-enable.hpp"

streamfx::ui::obs_browser_cef::obs_browser_cef()
{
	// Load the "obs-browser" module.
	_module = util::library::load(obs_get_module("obs-browser"));
	auto fn = reinterpret_cast<QCef* (*)(void)>(_module->load_symbol("obs_browser_create_qcef"));
	if (!fn) {
		throw std::runtime_error("Failed to load obs-browser module.");
	}

	// Create a QCef instance and initialize it.
	_cef = fn();
	if (!_cef) {
		throw std::runtime_error("Failed to create or get QCef instance.");
	}
	reinterpret_cast<QCef*>(_cef)->init_browser();
	reinterpret_cast<QCef*>(_cef)->wait_for_browser_init();

	// Create a generic Cookie manager for widgets.
	_cookie =
		reinterpret_cast<QCef*>(_cef)->create_cookie_manager(streamfx::config_file_path("cookies").u8string(), false);
}

streamfx::ui::obs_browser_cef::~obs_browser_cef()
{
	delete reinterpret_cast<QCefCookieManager*>(_cookie);
	delete reinterpret_cast<QCef*>(_cef);
}

void* streamfx::ui::obs_browser_cef::cef()
{
	return _cef;
}

void* streamfx::ui::obs_browser_cef::cookie_manager()
{
	return _cookie;
}

std::shared_ptr<streamfx::ui::obs_browser_cef> streamfx::ui::obs_browser_cef::instance()
{
	static std::weak_ptr<obs_browser_cef> ptr;
	static std::mutex                     lock;

	std::lock_guard<decltype(lock)> lg(lock);
	if (!ptr.expired()) {
		return ptr.lock();
	}

	std::shared_ptr<obs_browser_cef> sintance{new obs_browser_cef()};
	ptr = sintance;
	return sintance;
}

streamfx::ui::obs_browser_widget::obs_browser_widget(QUrl url, QWidget* parent) : QWidget(parent)
{
	// Create Layout
	auto layout = new QGridLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	// Create CEF Widget
	_cef    = obs_browser_cef::instance();
	_widget = reinterpret_cast<QCef*>(_cef->cef())
				  ->create_widget(this, url.toString().toStdString(),
								  reinterpret_cast<QCefCookieManager*>(_cef->cookie_manager()));
	if (!_widget) {
		throw std::runtime_error("Failed to create CEF Widget.");
	}
	dynamic_cast<QCefWidget*>(_widget)->allowAllPopups(false);
	_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	layout->addWidget(_widget, 0, 0);

	// This fixes a strange issue where Qt just does not update the size of the QCefWidget.
	// I do not know why this is, all I know is that this is absolutely necessary for QCefWidget.
	auto test = new QWidget(this);
	test->setFixedSize(0, 0);
	layout->addWidget(test, 1, 0);
}

streamfx::ui::obs_browser_widget::~obs_browser_widget() {}

QWidget* streamfx::ui::obs_browser_widget::cefwidget()
{
	return _widget;
}

void streamfx::ui::obs_browser_widget::set_url(QUrl url)
{
	dynamic_cast<QCefWidget*>(_widget)->setURL(url.toString().toStdString());
}

bool streamfx::ui::obs_browser_widget::is_available()
{
#ifdef D_PLATFORM_LINUX
	const char env_key[] = "XDG_SESSION_TYPE";
	const char wayland[] = "wayland";
#ifdef __STDC_LIB_EXT1__
	char   env_value[2048];
	size_t env_value_len = sizeof(env_value);
	if (getenv_s(&env_value_len, env_value, sizeof(env_key), env_key) == 0) {
		if (sizeof(wayland) == env_value_len) {
			if (strncmp(wayland, env_value, sizeof(wayland)) == 0) {
				return false;
			}
		}
	}
#else
	const char* env_value = getenv(env_key);
	if (strncmp(env_value, wayland, sizeof(wayland)) == 0) {
		return false;
	}
#endif
#endif
	return true;
}
