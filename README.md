# DX API Explorer


https://github.com/user-attachments/assets/39a573e8-dd27-4138-9664-4566b90eab43

[![Build and Release](https://github.com/BessmanPega/dx-api-explorer/actions/workflows/msbuild.yml/badge.svg)](https://github.com/BessmanPega/dx-api-explorer/actions/workflows/msbuild.yml)


This app helps you understand [Pega's Constellation DX API](https://docs.pega.com/bundle/dx-api/page/platform/dx-api/dx-api-version-2-con.html) by showing it in action. It's a desktop application that let's you work through a [case](https://docs.pega.com/bundle/platform/page/platform/case-management/case-management-overview.html), providing detailed developer information so you know *exactly* what the DX API is doing every step of the way.

## Key points

- Custom front-end to [Pega Infinity](https://www.pega.com/infinity) applications that use [Constellation UI](https://docs.pega.com/bundle/platform/page/platform/user-experience/constellation-architecture.html).
- Can work through a full [case lifecycle](https://docs.pega.com/bundle/platform/page/platform/case-management/case-life-cycle-elements.html), from creation to resolution.
- Complete request and response information for each API call.
- Inspectors for components, fields, and contents.
- Saves/loads configuration in `dx_api_explorer_config.json` — keep multiple versions on hand and swap them in to connect to different Pega instances.
- Startup and shutdown are nearly instantaneous, and uses almost zero CPU.
- Ships with all dependencies included, except for the [Microsoft Visual C++ Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe) (make sure you have this installed).
- Implemented in a straightforward funcs-and-structs style that readily translates to different languages and paradigms.
- No Windows-specific dependencies, should build on OSX and Linux with just a little bit of work.

## Usage

### Purpose
This is a developer tool, and a personal side project. It is not intended for production use, and has not been properly tested/hardened/etc — therefore no support is offered. It *is* intended to help you understand the DX API in particular, and [Pega's Center-out® architecture](https://www.pega.com/technology/center-out) in general.

### Installation

Just unzip it and go. You only need to ensure that you have a compatible Infinity application with an appropriately configured [OAuth 2.0 client registration](https://docs.pega.com/bundle/platform/page/platform/security/set-up-oauth-2-client-registration.html). It should work on any recent version of Infinity, but I have only tested it on 24.

### OAuth 2.0 client registration

You need to set this up in your Pega Infinity instance. It specifies how an external application can connect to Infinity. To get going quickly, here's the tl;dr—
- Important stuff:
	- Type of client: `Confidential`
	- Supported grant types: `Password credentials`
		- Identity mapping: `pyDefaultIdentityMappingForPasswordGrant`
		- Enable refresh token: `true`
	- **Click "View & Download" before save!**
- Other things to double check (but likely do not matter):
	- Refresh token: `Issue once and keep until expiry`
	- Access token lifetime (in seconds): `3600`
	- Refresh token lifetime (in seconds): `86400`
	- JWT generation profile: `pyDefaultUserInfoMapping`

### Connection

Upon startup, the application will present a form for connection information. If this is your first time running the application, there will be no config file and the form will be blank. Fill it out and and click `Login` to connect.

* Server: e.g. `https://my-pega-instance.example.com`.
* DX API Path: for default app, usually `/prweb/api/application/v2`.
	* To connect to a specific application, use `/prweb/app/{app-alias}/api/application/v2`
* Token Endpoint: usually `/prweb/PRRestService/oauth2/v1/token`.
* Client ID and Client Secret: obtained from the OAuth 2.0 client registration.
* User ID and Password: for [a supported user](https://docs.pega.com/bundle/dx-api/page/platform/dx-api/security-settings-v2.html).

Note that you will see information about the connection call in the debug window. This window will continuously update itself with each call. This shows you how to chain together DX API calls to get work done.

### Working through a case

Once connected, you will see a `Create` menu in the main window. This will allow you to refresh the list of available case types; having done that, you can then use it to create cases.

An open case shows buttons for its open assignments. Click a button to open an assignment. In turn, an open assignment shows buttons for opening the available actions.

An open action shows a form built out dynamically as specified by the DX API. Fill the form out and submit it to complete the assignment and close it out, updating the case information. Keep working in this way to progress through a complete case lifecycle.

### Debug window
The additional tabs in the debug window are very handy for programmatically understanding the structure, fields, and content.

In particular, the `Structure` tab is helpful for understanding how to interpret the `uiMetadata` portion of the DX API Response. Click on a component to select it and its JSON. For a visible component, this will also change the color of the info marker (the `(?)` icon) for that component in the main window. You can also click on the info marker to select the component.

### Customizing
The view menu lets you toggle additional windows. You can move and resize the windows, and the app will remember these settings. You can also return to the default layout at any time. And there's a toggle to enable an [XRay visualization akin to that provided by Constellation](https://docs.pega.com/bundle/platform/page/platform/user-experience/debugging-UI.html).

You can adjust the font size to taste. The app will attempt to select something reasonable on first run, but the current method is very crude and you will likely find that an alternative size is more suitable.

## Miscellaneous

### Current level of DX API support

A minimal set of components and field attributes are supported:

- Components:
	- Currency
	- DefaultForm
	- Reference (class context only)
	- Region
	- TextArea
	- TextInput
	- View

- Field attributes (**boolean only**):
	- Readonly
	- Required
	- Disabled

### Licensing and libraries

This app's code is released into the public domain. The included libraries all ship with unencumbered licenses. These libraries are:

- [SDL2](https://github.com/libsdl-org/SDL/tree/SDL2)
- [Dear ImGui](https://github.com/ocornut/imgui)
- [JSON for Modern C++](https://github.com/nlohmann/json)
- [cpp-httplib](https://github.com/yhirose/cpp-httplib)
- [OpenSSL](https://github.com/openssl/openssl)


### Contributing

Bugs are likely — issues and pull requests are welcome. As this is a side project and time is short, code which is not in harmony with the general style of the codebase will have to be rejected. Contributing to this project means assenting to said contributions being released into the public domain.

Thanks to [Daniël Wedema](https://github.com/danielwedema) for the idea to incorporate the X-Ray feature.

### Code structure

This project uses a [unity build](https://en.wikipedia.org/wiki/Unity_build) to speed up compilation. Code is mostly written directly in header files without function prototypes, which reduces the size of the codebase by more than you might expect. There are no global variables, although there *is* global state in [app_context_t](src/dx_api_app_types.h).

Component types are modeled with a [tagged union](https://en.wikipedia.org/wiki/Tagged_union) to keep the overall type system [simple](https://en.wikipedia.org/wiki/Complex_system). This also avoids virtual functions, which improves [cache locality](https://en.wikipedia.org/wiki/Locality_of_reference) and helps keep the [immediate mode GUI](https://caseymuratori.com/blog_0001) humming along at 60+ FPS without melting a CPU core.

For the sake of [locality of behavior](https://htmx.org/essays/locality-of-behaviour/), functions concerned with component types are collected in [src/dx_api_component_type_procs.cpp](src/dx_api_component_type_procs.cpp).

#### Adding a new component
This is perhaps best illustrated by example. Here's how I added support for the Currency component type:

##### 1. Study the component type
Using the DX API Explorer, I worked through a case with an assignment containing a currency field.

```json
{
  "config": {
    "allowDecimals": true,
    "alwaysShowISOCode": false,
    "isoCodeSelection": "constant",
    "label": "@FL .TransferAmount",
    "labelOption": "default",
    "value": "@P .TransferAmount"
  },
  "type": "Currency"
}
```

I wanted a very minimal implementation for this component — basically, treating it as a synonym for `TextInput` and letting Pega handle validation — so the only thing I needed here is the `type`, which is `Currency`.

##### 2. Update component types
In [src/dx_api_model_types.h](src/dx_api_model_types.h) I added a new entry to `component_type_t`:

```c++
//...
component_type_view,      // (5)
component_type_currency,  // (6) new entry
component_type_text_area, // (7)
//...
```

I then added a corresponding entry **at the same position** in `component_type_strings`:

```c++
//...
"View",     // (5)
"Currency", // (6) new entry
"TextArea", // (7)
//...
```

This latter entry is the value of `type` in the component's JSON definition.

##### 3. Update component type processing routines
At this point I then tried to build the project, which produced several warnings like so:

```
Warning	C4061:	enumerator 'dx_api_explorer::component_type_currency' in switch of enum 'dx_api_explorer::component_type_t' is not explicitly handled by a case label	
```
These warnings took me directly to locations in [src/dx_api_component_type_procs.cpp](src/dx_api_component_type_procs.cpp) where I needed to handle the newly added `component_type_currency`. Because I was just trying to provide a bare-minimum currency component for the time being, all I had to do was modify the relevant `switch` statements to handle `component_type_currency` in the same way as `component_type_text_input`.

```c++
case component_type_currency: // new entry
case component_type_text_area:
case component_type_text_input:
```

おしまい。

Of course, more sophisticated components will require more work. More on that anon.
