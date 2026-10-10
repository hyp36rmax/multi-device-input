#!/usr/bin/env ruby
require 'json'
require 'fileutils'

ROOT = File.expand_path(__dir__)
NAME = 'HYP36rforce Digital Dash'
OUT = File.join(ROOT, NAME)
FileUtils.mkdir_p(OUT)

C = {
  navy: '#FF081322', black: '#FF05080F', orange: '#FFFF8A30', pink: '#FFFF3D81',
  cyan: '#FF24D8E8', turquoise: '#FF16B8B4', white: '#FFF4F5F7', muted: '#FF8295A8',
  panel: '#E6102032', track: '#FF17283A', transparent: '#00FFFFFF'
}

def bind(expression, target, format = nil)
  value = { 'Formula' => { 'Interpreter' => 1, 'Expression' => expression }, 'Mode' => 2,
            'TargetPropertyName' => target }
  value['FormatString'] = format if format
  value
end

def rect(name, x, y, width, height, color, border = nil, bindings = nil)
  item = { '$type' => 'SimHub.Plugins.OutputPlugins.GraphicalDash.Models.RectangleItem, SimHub.Plugins',
           'IsRectangleItem' => true, 'BackgroundColor' => color, 'Height' => height.to_f,
           'Left' => x.to_f, 'Top' => y.to_f, 'Visible' => true, 'Width' => width.to_f, 'Name' => name }
  item['BorderStyle'] = { 'BorderColor' => border, 'BorderTop' => 2, 'BorderBottom' => 2,
                          'BorderLeft' => 2, 'BorderRight' => 2 } if border
  item['Bindings'] = bindings if bindings
  item
end

def text(name, value, x, y, width, height, size, color, align = 0, weight = 'Regular', bindings = nil)
  item = { '$type' => 'SimHub.Plugins.OutputPlugins.GraphicalDash.Models.TextItem, SimHub.Plugins',
           'IsTextItem' => true, 'Font' => 'Segoe UI', 'FontWeight' => weight,
           'FontSize' => size.to_f, 'Text' => value, 'TextColor' => color,
           'HorizontalAlignment' => align, 'VerticalAlignment' => 1,
           'BackgroundColor' => C[:transparent], 'Height' => height.to_f, 'Left' => x.to_f,
           'Top' => y.to_f, 'Visible' => true, 'Width' => width.to_f, 'Name' => name }
  item['Bindings'] = bindings if bindings
  item
end

def text_binding(property, fallback = '--', digits = nil, suffix = '')
  format = digits.nil? ? 'String(v)' : "Number(v).toFixed(#{digits})"
  "var v=$prop('#{property}'); if(v===null || v===undefined || v==='' || !isFinite(Number(v))) return '#{fallback}'; return #{format}+'#{suffix}';"
end

items = []
items << rect('Background', 0, 0, 1920, 1080, C[:black])
items << rect('TopBand', 0, 0, 1920, 184, C[:navy])
items << rect('SunsetLineOrange', 0, 180, 1280, 4, C[:orange])
items << rect('SunsetLinePink', 1280, 180, 640, 4, C[:pink])
items << text('Brand', 'HYP36rforce', 70, 34, 620, 72, 48, C[:white], 0, 'Bold')
items << text('Subtitle', 'OUTRUN 2006 TELEMETRY', 74, 104, 620, 38, 22, C[:cyan], 0, 'SemiBold')
items << text('CarLabel', 'CURRENT CAR', 860, 26, 930, 30, 18, C[:muted], 2, 'SemiBold')
items << text('CarName', '--', 760, 58, 1030, 52, 30, C[:white], 2, 'SemiBold', {
  'Text' => bind("var v=$prop('DataCorePlugin.GameRawData.Custom_CarName'); return v ? String(v) : '--';", 'Text')
})
items << text('StageLabel', 'NATIVE STAGE', 860, 116, 740, 26, 16, C[:muted], 2, 'SemiBold')
items << text('StageTop', '--', 1610, 110, 180, 40, 24, C[:cyan], 2, 'Bold', {
  'Text' => bind("var v=$prop('DataCorePlugin.GameRawData.Custom_StageID'); return Number(v)>=0 ? String(v) : '--';", 'Text')
})

items << rect('CenterPanel', 70, 230, 1780, 390, C[:panel], C[:track])
items << text('SpeedLabel', 'SPEED', 220, 270, 760, 34, 19, C[:muted], 1, 'SemiBold')
items << text('Speed', '--', 180, 302, 840, 210, 150, C[:white], 1, 'Bold', {
  'Text' => bind("var v=Number($prop('SpeedKmh')); return isFinite(v)&&v>0 ? Math.round(v).toString() : '--';", 'Text')
})
items << text('SpeedUnit', 'km/h', 390, 508, 420, 42, 25, C[:cyan], 1, 'SemiBold')
items << rect('CenterDivider', 1090, 270, 3, 280, C[:track])
items << text('GearLabel', 'GEAR', 1210, 270, 470, 34, 19, C[:muted], 1, 'SemiBold')
items << text('Gear', '--', 1160, 308, 570, 210, 150, C[:orange], 1, 'Bold', {
  'Text' => bind("var v=$prop('Gear'); return v===null||v===undefined||String(v)==='' ? '--' : String(v);", 'Text')
})

# Decorative segmented arc: visual rhythm only, deliberately not bound to RPM.
12.times do |i|
  color = i < 7 ? C[:cyan] : (i < 10 ? C[:orange] : C[:pink])
  items << rect("DecorativeSegment#{i + 1}", 570 + i * 67, 574, 48, 7, color)
end

items << text('SteeringTitle', 'STEERING INPUT', 90, 670, 740, 32, 18, C[:muted], 0, 'SemiBold')
items << text('SteeringValue', '0.000', 600, 665, 230, 40, 24, C[:cyan], 2, 'Bold', {
  'Text' => bind(text_binding('DataCorePlugin.GameRawData.Custom_SteeringInput', '--', 3), 'Text')
})
items << rect('SteeringTrack', 90, 720, 740, 34, C[:track])
items << rect('SteeringCenter', 459, 711, 2, 52, C[:white])
items << rect('SteeringLeft', 90, 720, 370, 34, C[:cyan], nil, {
  'Left' => bind("var v=Number($prop('DataCorePlugin.GameRawData.Custom_SteeringInput')); if(!isFinite(v)||v>=0)return 460; return 460-Math.min(1,Math.abs(v))*370;", 'Left'),
  'Width' => bind("var v=Number($prop('DataCorePlugin.GameRawData.Custom_SteeringInput')); return !isFinite(v)||v>=0 ? 0 : Math.min(1,Math.abs(v))*370;", 'Width')
})
items << rect('SteeringRight', 460, 720, 370, 34, C[:cyan], nil, {
  'Width' => bind("var v=Number($prop('DataCorePlugin.GameRawData.Custom_SteeringInput')); return !isFinite(v)||v<=0 ? 0 : Math.min(1,v)*370;", 'Width')
})
items << text('SteeringMin', '-1.0', 90, 760, 100, 24, 14, C[:muted])
items << text('SteeringZero', '0', 410, 760, 100, 24, 14, C[:muted], 1)
items << text('SteeringMax', '+1.0', 730, 760, 100, 24, 14, C[:muted], 2)

items << text('RoadTitle', 'ROAD ACTIVITY', 900, 670, 400, 32, 18, C[:muted], 0, 'SemiBold')
items << text('RoadValue', '0.000', 1120, 665, 180, 40, 24, C[:turquoise], 2, 'Bold', {
  'Text' => bind(text_binding('DataCorePlugin.GameRawData.Custom_RoadActivity', '--', 3), 'Text')
})
items << rect('RoadTrack', 900, 720, 400, 34, C[:track])
items << rect('RoadFill', 900, 720, 400, 34, C[:turquoise], nil, {
  'Width' => bind("var v=Number($prop('DataCorePlugin.GameRawData.Custom_RoadActivity')); return isFinite(v)?Math.max(0,Math.min(1,v))*400:0;", 'Width')
})
items << text('RoadNote', 'NATIVE EFFECT ACTIVITY • 0–1', 900, 760, 400, 24, 13, C[:muted])

items << text('ImpactTitle', 'IMPACT INTENSITY', 1370, 670, 400, 32, 18, C[:muted], 0, 'SemiBold')
items << text('ImpactValue', '0.000', 1590, 665, 180, 40, 24, C[:orange], 2, 'Bold', {
  'Text' => bind(text_binding('DataCorePlugin.GameRawData.Custom_ImpactIntensity', '--', 3), 'Text')
})
items << rect('ImpactTrack', 1370, 720, 400, 34, C[:track])
items << rect('ImpactFillOrange', 1370, 720, 400, 34, C[:orange], nil, {
  'Width' => bind("var v=Number($prop('DataCorePlugin.GameRawData.Custom_ImpactIntensity')); return isFinite(v)?Math.max(0,Math.min(1,v))*400:0;", 'Width')
})
items << rect('ImpactFillPink', 1650, 720, 120, 34, C[:pink], nil, {
  'Visible' => bind("var v=Number($prop('DataCorePlugin.GameRawData.Custom_ImpactIntensity')); return isFinite(v)&&v>=0.7;", 'Visible')
})
items << text('ImpactNote', 'NATIVE COMPOSITE EVIDENCE • 0–1', 1370, 760, 400, 24, 13, C[:muted])

items << rect('DiagnosticPanel', 70, 850, 1780, 160, C[:navy], C[:track])
items << text('DiagnosticTitle', 'TELEMETRY DIAGNOSTICS', 100, 875, 430, 30, 18, C[:pink], 0, 'Bold')
items << text('TelemetryState', 'Telemetry State: Unverified', 100, 920, 430, 40, 22, C[:white], 0, 'SemiBold')
diag = [
  ['CAR ID', 'DataCorePlugin.GameRawData.Custom_CarID', 600, 0],
  ['STAGE ID', 'DataCorePlugin.GameRawData.Custom_StageID', 820, 0],
  ['STEERING', 'DataCorePlugin.GameRawData.Custom_SteeringInput', 1040, 3],
  ['ROAD', 'DataCorePlugin.GameRawData.Custom_RoadActivity', 1280, 3],
  ['IMPACT', 'DataCorePlugin.GameRawData.Custom_ImpactIntensity', 1500, 3]
]
diag.each do |label, property, x, digits|
  items << text("Diag#{label}Label", label, x, 880, 190, 24, 14, C[:muted], 1, 'SemiBold')
  expr = digits.zero? ? "var v=Number($prop('#{property}')); return isFinite(v)&&v>=0?String(Math.round(v)):'--';" : text_binding(property, '--', digits)
  items << text("Diag#{label}", '--', x, 920, 190, 38, 22, C[:white], 1, 'Bold', { 'Text' => bind(expr, 'Text') })
end
items << text('Footer', 'LIVE VALUES ARE PROVIDED BY THE HYP36rforce SIMHUB ADAPTER • NO SYNTHETIC PHYSICS', 70, 1030, 1780, 24, 13, C[:muted], 1, 'SemiBold')

dashboard = {
  'Variables' => { 'DashboardVariables' => [] }, 'DashboardDebugManager' => { '_dashSettingsStores' => {} },
  'Version' => 2, 'Id' => '07a0c694-196d-4e74-a12d-7a61ac090b74', 'BaseHeight' => 1080,
  'BaseWidth' => 1920, 'BackgroundColor' => C[:black],
  'Screens' => [{ 'RenderingSkip' => 0, 'Name' => 'Main', 'InGameScreen' => true, 'IdleScreen' => true,
    'PitScreen' => false, 'ScreenId' => '2ea91585-31e6-4e66-b770-b1009157383f', 'AllowOverlays' => true,
    'IsForegroundLayer' => false, 'IsOverlayLayer' => false, 'OverlayTriggerExpression' => { 'Expression' => '' },
    'ScreenEnabledExpression' => { 'Expression' => '' }, 'OverlayMaxDuration' => 0, 'OverlayMinDuration' => 0,
    'IsBackgroundLayer' => false, 'BackgroundColor' => C[:black], 'Items' => items, 'MinimumRefreshIntervalMS' => 0.0 }],
  'SnapToGrid' => false, 'HideLabels' => false, 'ShowForeground' => true, 'ForegroundOpacity' => 100.0,
  'ShowBackground' => true, 'BackgroundOpacity' => 100.0, 'ShowBoundingRectangles' => false, 'GridSize' => 10,
  'Images' => [], 'Metadata' => { 'SettingsBuilder' => { 'Settings' => [], 'IsEditMode' => false },
    'ScreenCount' => 1.0, 'InGameScreensIndexs' => [0], 'IdleScreensIndexs' => [0], 'MainPreviewIndex' => 0,
    'IsOverlay' => false, 'OverlaySizeWarning' => false, 'MetadataVersion' => 2.0,
    'EnableOnDashboardMessaging' => true, 'PitScreensIndexs' => [], 'PreferredTouchMode' => 0,
    'SimHubVersion' => '9.13.2', 'Width' => 1920.0, 'Height' => 1080.0, 'DashboardVersion' => '1.0.0' },
  'ShowOnScreenControls' => false, 'IsOverlay' => false, 'EnableClickThroughOverlay' => false,
  'EnableOnDashboardMessaging' => true, 'UseStrictJSIsolation' => false, 'UseStrictJSIsolationWarning' => true
}

metadata = dashboard['Metadata'].merge({ 'Prefix' => nil, 'Category' => 'HYP36rforce',
  'Title' => NAME, 'Description' => 'OutRun 2006 telemetry instruments and live property diagnostics.',
  'Author' => 'hyp36rmax' })

File.write(File.join(OUT, "#{NAME}.djson"), JSON.pretty_generate(dashboard))
File.write(File.join(OUT, "#{NAME}.djson.metadata"), JSON.pretty_generate(metadata))
