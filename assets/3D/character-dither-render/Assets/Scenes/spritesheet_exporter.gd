@tool
extends Node3D

enum DitherPattern { BAYER, CLUSTERED_DOT, BLUE_NOISE, FLOYD_STEINBERG, ATKINSON }
enum EaseFunction {
	LINEAR,
	EASE_IN_QUAD, EASE_OUT_QUAD, EASE_IN_OUT_QUAD,
	EASE_IN_CUBIC, EASE_OUT_CUBIC, EASE_IN_OUT_CUBIC,
	EASE_IN_QUART, EASE_OUT_QUART, EASE_IN_OUT_QUART,
	EASE_IN_BACK, EASE_OUT_BACK, EASE_IN_OUT_BACK,
}

@export var animation_tree_path: NodePath = ^"../AnimationTree"
@export var camera_path: NodePath = ^"../Camera3D"
@export var state_machine_param_path: String = "parameters/StateMachine/playback"
@export var initial_state_name: String = ""
@export var final_state_name: String = ""
@export var fps: int = 40
@export var viewport_width: int = 200
@export var viewport_height: int = 120
@export_range(1, 4, 1) var supersample: int = 2
@export var dither_pattern: DitherPattern = DitherPattern.BAYER
@export var bayer_size: int = 8
@export var blue_noise_texture: Texture2D
@export_range(-1.0, 1.0, 0.01) var threshold_offset: float = 0.0
@export_range(0.0, 1.0, 0.01) var alpha_cutoff: float = 0.5
@export_range(0.1, 4.0, 0.05) var contrast_gamma: float = 1.0
@export_range(0, 8, 1) var cel_bands: int = 0
@export_range(0.0, 1.0, 0.01) var edge_strength: float = 0.0

@export_group("Tone Bands")
@export_range(0, 16, 1) var tone_bands: int = 0
@export_range(0, 5, 1) var tone_blur: int = 0
@export_group("")

@export_group("Outline")
@export var outline_enabled: bool = false
@export_range(0.0, 0.1, 0.001) var outline_thickness: float = 0.02
@export var outline_color: Color = Color.BLACK
@export_group("")

@export_group("Silhouette")
@export var silhouette_enabled: bool = false
@export_range(0, 10, 1) var silhouette_thickness: int = 2
@export var silhouette_color: Color = Color.WHITE
@export_group("")

@export_group("Cel Shader")
@export var cel_material: Material : set = _set_cel_material
@export var cel_enabled: bool = false : set = _set_cel_enabled
@export_group("")

@export_group("Initial Rotation")
@export var rotation_enabled: bool = true
@export var rotation_target_path: NodePath = ^"../Rig_Medium"
@export_range(-360.0, 360.0, 1.0) var rotation_start_degrees: float = 180.0
@export_range(0.0, 5.0, 0.05) var rotation_duration: float = 0.75
@export var rotation_ease: EaseFunction = EaseFunction.EASE_OUT_CUBIC
@export_group("")

@export var max_capture_seconds: float = 30.0

@export_tool_button("Preview First Frame", "Camera3D") var _preview_btn: Callable = _preview_first_frame
@export var preview_texture: Texture2D

const BAYER_2: Array[int] = [
	0, 2,
	3, 1,
]
const BAYER_4: Array[int] = [
	 0,  8,  2, 10,
	12,  4, 14,  6,
	 3, 11,  1,  9,
	15,  7, 13,  5,
]
const BAYER_8: Array[int] = [
	 0, 32,  8, 40,  2, 34, 10, 42,
	48, 16, 56, 24, 50, 18, 58, 26,
	12, 44,  4, 36, 14, 46,  6, 38,
	60, 28, 52, 20, 62, 30, 54, 22,
	 3, 35, 11, 43,  1, 33,  9, 41,
	51, 19, 59, 27, 49, 17, 57, 25,
	15, 47,  7, 39, 13, 45,  5, 37,
	63, 31, 55, 23, 61, 29, 53, 21,
]
const CLUSTERED_4: Array[int] = [
	 0,  4,  5,  1,
	 6, 12, 13,  7,
	 8, 14, 15,  9,
	 2, 10, 11,  3,
]
const CLUSTERED_8: Array[int] = [
	 0, 16, 20,  8,  9, 17, 21,  1,
	44, 48, 56, 24, 45, 49, 57, 25,
	40, 60, 52, 28, 41, 61, 53, 29,
	12, 36, 32,  4,  5, 37, 33, 13,
	14, 18, 22,  6,  7, 19, 23, 15,
	46, 50, 58, 26, 47, 51, 59, 27,
	42, 62, 54, 30, 43, 63, 55, 31,
	 2, 38, 34, 10, 11, 39, 35,  3,
]

const OUTLINE_SHADER_CODE := """
shader_type spatial;
render_mode unshaded, cull_front, depth_draw_opaque;

uniform float thickness : hint_range(0.0, 0.5) = 0.02;
uniform vec4 outline_color : source_color = vec4(0.0, 0.0, 0.0, 1.0);

void vertex() {
	VERTEX += NORMAL * thickness;
}

void fragment() {
	ALBEDO = outline_color.rgb;
	ALPHA = outline_color.a;
}
"""

var _outlined_meshes: Array[MeshInstance3D] = []
var _cel_meshes: Array[MeshInstance3D] = []


func _ready() -> void:
	if Engine.is_editor_hint():
		if cel_enabled:
			_apply_cel_material()
		return
	await get_tree().process_frame
	await _export()
	get_tree().quit()


func _exit_tree() -> void:
	if Engine.is_editor_hint():
		_remove_cel_material()


func _set_cel_enabled(v: bool) -> void:
	cel_enabled = v
	if not is_inside_tree():
		return
	if v:
		_apply_cel_material()
	else:
		_remove_cel_material()


func _set_cel_material(m: Material) -> void:
	cel_material = m
	if not is_inside_tree() or not cel_enabled:
		return
	_apply_cel_material()


func _apply_cel_material() -> void:
	_remove_cel_material()
	if cel_material == null:
		return
	var root := get_parent()
	if root == null:
		return
	_apply_cel_material_recursive(root)


func _apply_cel_material_recursive(node: Node) -> void:
	if node is MeshInstance3D and not node.get_parent() is BoneAttachment3D:
		(node as MeshInstance3D).material_override = cel_material
		_cel_meshes.append(node)
	for child in node.get_children():
		_apply_cel_material_recursive(child)


func _remove_cel_material() -> void:
	for mi in _cel_meshes:
		if is_instance_valid(mi):
			mi.material_override = null
	_cel_meshes.clear()


func _preview_first_frame() -> void:
	if not Engine.is_editor_hint():
		return
	var source_camera := get_node_or_null(camera_path) as Camera3D
	if source_camera == null:
		push_error("SpritesheetExporter: camera not found at '%s'" % camera_path)
		return

	var sv := SubViewport.new()
	sv.size = Vector2i(viewport_width * supersample, viewport_height * supersample)
	sv.transparent_bg = true
	sv.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	sv.msaa_3d = Viewport.MSAA_DISABLED
	add_child(sv)

	var cam := Camera3D.new()
	sv.add_child(cam)
	cam.global_transform = source_camera.global_transform
	cam.fov = source_camera.fov
	cam.near = source_camera.near
	cam.far = source_camera.far
	cam.projection = source_camera.projection
	cam.size = source_camera.size
	cam.keep_aspect = source_camera.keep_aspect
	cam.current = true

	_apply_outline()
	await RenderingServer.frame_post_draw
	await RenderingServer.frame_post_draw

	var img: Image = sv.get_texture().get_image()
	sv.queue_free()
	_remove_outline()

	if supersample > 1:
		img.resize(viewport_width, viewport_height, Image.INTERPOLATE_LANCZOS)
	_dither_frame(img)
	_apply_silhouette(img)
	preview_texture = ImageTexture.create_from_image(img)
	notify_property_list_changed()


func _export() -> void:
	var tree: AnimationTree = get_node(animation_tree_path)
	var source_camera: Camera3D = get_node(camera_path)

	var sv := SubViewport.new()
	sv.size = Vector2i(viewport_width * supersample, viewport_height * supersample)
	sv.transparent_bg = true
	sv.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	sv.msaa_3d = Viewport.MSAA_DISABLED
	add_child(sv)

	var cam := Camera3D.new()
	sv.add_child(cam)
	cam.global_transform = source_camera.global_transform
	cam.fov = source_camera.fov
	cam.near = source_camera.near
	cam.far = source_camera.far
	cam.projection = source_camera.projection
	cam.size = source_camera.size
	cam.keep_aspect = source_camera.keep_aspect
	cam.current = true

	var state_machine := _resolve_state_machine(tree)
	if state_machine == null:
		push_error("SpritesheetExporter: could not resolve state machine at '%s'" % state_machine_param_path)
		return
	var playback_path := _resolve_playback_path(tree)

	var initial := initial_state_name
	if initial.is_empty():
		initial = _find_initial_state(state_machine)
	var final := final_state_name
	if final.is_empty():
		final = _find_final_state(state_machine)
	if initial.is_empty() or final.is_empty():
		push_error("SpritesheetExporter: could not auto-detect states (initial='%s' final='%s')" % [initial, final])
		return
	print("SpritesheetExporter: initial='%s' final='%s'" % [initial, final])

	tree.active = true
	tree.callback_mode_process = AnimationTree.ANIMATION_CALLBACK_MODE_PROCESS_MANUAL
	var playback: AnimationNodeStateMachinePlayback = tree.get(playback_path)
	playback.start(initial)
	tree.advance(0.0)

	_apply_outline()

	var rotation_target: Node3D = null
	var rotation_original := Vector3.ZERO
	if rotation_enabled and not rotation_target_path.is_empty():
		rotation_target = get_node_or_null(rotation_target_path) as Node3D
		if rotation_target == null:
			push_warning("SpritesheetExporter: rotation_target not found at '%s'" % rotation_target_path)
		else:
			rotation_original = rotation_target.rotation
			rotation_target.rotation = Vector3(rotation_original.x, rotation_original.y + deg_to_rad(rotation_start_degrees), rotation_original.z)

	var dt := 1.0 / float(fps)
	var frames: Array[Image] = []
	var elapsed := 0.0
	var final_state_elapsed := -1.0
	var final_state_length := 0.0
	var rotation_started := false
	var rotation_elapsed := 0.0
	var initial_buffer: Image = null
	var inserted_loop_marker := false

	while elapsed < max_capture_seconds:
		await RenderingServer.frame_post_draw
		var frame_img: Image = sv.get_texture().get_image()
		if supersample > 1:
			frame_img.resize(viewport_width, viewport_height, Image.INTERPOLATE_LANCZOS)

		var current := String(playback.get_current_node())

		if current == initial:
			initial_buffer = frame_img
		else:
			if initial_buffer != null:
				frames.append(initial_buffer)
				initial_buffer = null
			frames.append(frame_img)

		if rotation_target != null:
			if not rotation_started and current != initial:
				rotation_started = true
			var rt: float
			if not rotation_started:
				rt = 0.0
			elif rotation_duration <= 0.0:
				rt = 1.0
			else:
				rt = clampf(rotation_elapsed / rotation_duration, 0.0, 1.0)
			var eased := _apply_ease(rt, rotation_ease)
			var offset_deg: float = lerp(rotation_start_degrees, 0.0, eased)
			rotation_target.rotation = Vector3(rotation_original.x, rotation_original.y + deg_to_rad(offset_deg), rotation_original.z)
			if rotation_started:
				rotation_elapsed += dt

		if current == final:
			if final_state_elapsed < 0.0:
				final_state_elapsed = 0.0
				final_state_length = playback.get_current_length()
			else:
				final_state_elapsed += dt
			if final_state_length > 0.0 and final_state_elapsed >= final_state_length:
				if not inserted_loop_marker:
					var marker := Image.create(viewport_width, viewport_height, false, Image.FORMAT_RGBA8)
					marker.fill(Color(0, 0, 0, 0))
					frames.append(marker)
					inserted_loop_marker = true
					final_state_elapsed = 0.0
				else:
					break

		tree.advance(dt)
		elapsed += dt

	_remove_outline()
	if rotation_target != null:
		rotation_target.rotation = rotation_original

	if frames.is_empty():
		push_error("SpritesheetExporter: no frames captured.")
		return
	print("SpritesheetExporter: captured %d raw frames; dithering..." % frames.size())

	for img in frames:
		_dither_frame(img)
		_apply_silhouette(img)

	var union_rect := Rect2i()
	for img in frames:
		var r := img.get_used_rect()
		if r.size == Vector2i.ZERO:
			continue
		if union_rect.size == Vector2i.ZERO:
			union_rect = r
		else:
			union_rect = union_rect.merge(r)
	if union_rect.size == Vector2i.ZERO:
		push_error("SpritesheetExporter: all frames are fully transparent.")
		return

	var fw := union_rect.size.x
	var fh := union_rect.size.y
	var sheet := Image.create(fw * frames.size(), fh, false, Image.FORMAT_RGBA8)
	sheet.fill(Color(0, 0, 0, 0))
	for i in frames.size():
		sheet.blit_rect(frames[i], union_rect, Vector2i(i * fw, 0))

	var scene_path: String = get_tree().current_scene.scene_file_path
	var scene_name: String = scene_path.get_file().get_basename()
	var output_dir := "res://Assets/Exports"
	DirAccess.make_dir_recursive_absolute(output_dir)
	var output_path := "%s/%s-table-%d-%d.png" % [output_dir, scene_name, fw, fh]
	var err := sheet.save_png(output_path)
	if err != OK:
		push_error("SpritesheetExporter: save_png failed (%d) at %s" % [err, output_path])
		return
	print("SpritesheetExporter: exported %d frames -> %s (%dx%d per frame)" % [frames.size(), output_path, fw, fh])


func _resolve_state_machine(tree: AnimationTree) -> AnimationNodeStateMachine:
	var root := tree.tree_root
	if root is AnimationNodeStateMachine:
		return root as AnimationNodeStateMachine
	var parts := state_machine_param_path.split("/")
	if parts.size() < 2:
		return null
	var node: AnimationNode = root
	for i in range(1, parts.size() - 1):
		var part := parts[i]
		if node is AnimationNodeBlendTree:
			node = (node as AnimationNodeBlendTree).get_node(part)
		elif node is AnimationNodeStateMachine:
			node = (node as AnimationNodeStateMachine).get_node(part)
		else:
			return null
		if node == null:
			return null
	return node as AnimationNodeStateMachine


func _resolve_playback_path(tree: AnimationTree) -> String:
	if tree.tree_root is AnimationNodeStateMachine:
		return "parameters/playback"
	return state_machine_param_path


func _find_initial_state(sm: AnimationNodeStateMachine) -> String:
	for i in sm.get_transition_count():
		if String(sm.get_transition_from(i)) == "Start":
			return String(sm.get_transition_to(i))
	return ""


func _find_final_state(sm: AnimationNodeStateMachine) -> String:
	var sources := {}
	var seen := {}
	for i in sm.get_transition_count():
		var from_state := String(sm.get_transition_from(i))
		var to_state := String(sm.get_transition_to(i))
		sources[from_state] = true
		seen[from_state] = true
		seen[to_state] = true
	var candidates: Array[String] = []
	for sname in seen.keys():
		if sname == "Start" or sname == "End":
			continue
		if not sources.has(sname):
			candidates.append(sname)
	if candidates.size() == 1:
		return candidates[0]
	if sm.get_transition_count() > 0:
		return String(sm.get_transition_to(sm.get_transition_count() - 1))
	return ""


func _dither_frame(img: Image) -> void:
	var size := img.get_size()
	var w := size.x
	var h := size.y

	var luma_grid := PackedFloat32Array()
	luma_grid.resize(w * h)
	for y in h:
		for x in w:
			var c := img.get_pixel(x, y)
			luma_grid[y * w + x] = 0.299 * c.r + 0.587 * c.g + 0.114 * c.b

	if tone_blur > 0 and tone_bands >= 2:
		luma_grid = _box_blur_luma(luma_grid, w, h, tone_blur)

	var effective_pattern := dither_pattern
	if tone_bands >= 2 and (effective_pattern == DitherPattern.FLOYD_STEINBERG or effective_pattern == DitherPattern.ATKINSON):
		effective_pattern = DitherPattern.BAYER

	var edge_threshold := 1.0 - edge_strength
	if effective_pattern == DitherPattern.FLOYD_STEINBERG or effective_pattern == DitherPattern.ATKINSON:
		var work := PackedFloat32Array()
		work.resize(w * h)
		for i in luma_grid.size():
			var l: float = luma_grid[i]
			if contrast_gamma != 1.0:
				l = pow(l, contrast_gamma)
			if cel_bands >= 2:
				var band: int = clampi(int(l * cel_bands), 0, cel_bands - 1)
				l = float(band) / float(cel_bands - 1)
			work[i] = l
		var threshold_ed := 0.5 - threshold_offset
		for y in h:
			for x in w:
				var c := img.get_pixel(x, y)
				if c.a < alpha_cutoff:
					img.set_pixel(x, y, Color(0, 0, 0, 0))
					continue
				if edge_strength > 0.0 and _sobel_at(luma_grid, x, y, w, h) > edge_threshold:
					img.set_pixel(x, y, Color(0, 0, 0, 1))
					continue
				var idx := y * w + x
				var old: float = work[idx]
				var new_val: float = 1.0 if old > threshold_ed else 0.0
				var err: float = old - new_val
				if dither_pattern == DitherPattern.FLOYD_STEINBERG:
					if x + 1 < w:
						work[idx + 1] += err * 0.4375
					if y + 1 < h:
						if x > 0:
							work[idx + w - 1] += err * 0.1875
						work[idx + w] += err * 0.3125
						if x + 1 < w:
							work[idx + w + 1] += err * 0.0625
				else:
					var r: float = err * 0.125
					if x + 1 < w:
						work[idx + 1] += r
					if x + 2 < w:
						work[idx + 2] += r
					if y + 1 < h:
						if x > 0:
							work[idx + w - 1] += r
						work[idx + w] += r
						if x + 1 < w:
							work[idx + w + 1] += r
					if y + 2 < h:
						work[idx + 2 * w] += r
				if new_val > 0.5:
					img.set_pixel(x, y, Color(1, 1, 1, 1))
				else:
					img.set_pixel(x, y, Color(0, 0, 0, 1))
		return

	var matrix: Array[int] = []
	var divisor: float = 1.0
	var n: int = 0
	var bn_img: Image = null
	var bn_w: int = 0
	var bn_h: int = 0

	match effective_pattern:
		DitherPattern.BAYER:
			match bayer_size:
				2: matrix = BAYER_2; divisor = 4.0
				4: matrix = BAYER_4; divisor = 16.0
				_: matrix = BAYER_8; divisor = 64.0
			n = int(sqrt(matrix.size()))
		DitherPattern.CLUSTERED_DOT:
			match bayer_size:
				4: matrix = CLUSTERED_4; divisor = 16.0
				_: matrix = CLUSTERED_8; divisor = 64.0
			n = int(sqrt(matrix.size()))
		DitherPattern.BLUE_NOISE:
			if blue_noise_texture != null:
				bn_img = blue_noise_texture.get_image()
				if bn_img != null:
					bn_w = bn_img.get_width()
					bn_h = bn_img.get_height()
			if bn_img == null:
				push_warning("SpritesheetExporter: blue_noise_texture not set; falling back to Bayer 8")
				matrix = BAYER_8
				divisor = 64.0
				n = 8

	for y in h:
		for x in w:
			var c := img.get_pixel(x, y)
			if c.a < alpha_cutoff:
				img.set_pixel(x, y, Color(0, 0, 0, 0))
				continue
			if edge_strength > 0.0 and _sobel_at(luma_grid, x, y, w, h) > edge_threshold:
				img.set_pixel(x, y, Color(0, 0, 0, 1))
				continue
			var luma: float = luma_grid[y * w + x]
			if contrast_gamma != 1.0:
				luma = pow(luma, contrast_gamma)
			if tone_bands >= 2:
				var band: int = clampi(int(luma * tone_bands), 0, tone_bands - 1)
				luma = float(band) / float(tone_bands - 1)
			elif cel_bands >= 2:
				var band: int = clampi(int(luma * cel_bands), 0, cel_bands - 1)
				luma = float(band) / float(cel_bands - 1)
			var threshold: float
			if bn_img != null:
				threshold = bn_img.get_pixel(x % bn_w, y % bn_h).r + threshold_offset
			else:
				threshold = float(matrix[(y % n) * n + (x % n)]) / divisor + threshold_offset
			if luma > threshold:
				img.set_pixel(x, y, Color(1, 1, 1, 1))
			else:
				img.set_pixel(x, y, Color(0, 0, 0, 1))


func _apply_silhouette(img: Image) -> void:
	if not silhouette_enabled or silhouette_thickness <= 0:
		return
	var w := img.get_width()
	var h := img.get_height()
	var mask := PackedByteArray()
	mask.resize(w * h)
	for y in h:
		for x in w:
			mask[y * w + x] = 1 if img.get_pixel(x, y).a > 0.0 else 0
	var t := silhouette_thickness
	var t_sq := t * t
	for y in h:
		for x in w:
			if mask[y * w + x] != 0:
				continue
			var hit := false
			for dy in range(-t, t + 1):
				if hit:
					break
				var ny := y + dy
				if ny < 0 or ny >= h:
					continue
				for dx in range(-t, t + 1):
					if dx * dx + dy * dy > t_sq:
						continue
					var nx := x + dx
					if nx < 0 or nx >= w:
						continue
					if mask[ny * w + nx] != 0:
						hit = true
						break
			if hit:
				img.set_pixel(x, y, silhouette_color)


func _apply_outline() -> void:
	_remove_outline()
	if not outline_enabled:
		return
	var shader := Shader.new()
	shader.code = OUTLINE_SHADER_CODE
	var mat := ShaderMaterial.new()
	mat.shader = shader
	mat.set_shader_parameter("thickness", outline_thickness)
	mat.set_shader_parameter("outline_color", outline_color)
	_apply_outline_recursive(get_parent(), mat)


func _apply_outline_recursive(node: Node, mat: Material) -> void:
	if node is MeshInstance3D:
		(node as MeshInstance3D).material_overlay = mat
		_outlined_meshes.append(node)
	for child in node.get_children():
		_apply_outline_recursive(child, mat)


func _remove_outline() -> void:
	for mi in _outlined_meshes:
		if is_instance_valid(mi):
			mi.material_overlay = null
	_outlined_meshes.clear()


func _box_blur_luma(src: PackedFloat32Array, w: int, h: int, r: int) -> PackedFloat32Array:
	var out := PackedFloat32Array()
	out.resize(w * h)
	for y in h:
		for x in w:
			var sum := 0.0
			var cnt := 0
			for dy in range(-r, r + 1):
				var ny := y + dy
				if ny < 0 or ny >= h:
					continue
				for dx in range(-r, r + 1):
					var nx := x + dx
					if nx < 0 or nx >= w:
						continue
					sum += src[ny * w + nx]
					cnt += 1
			out[y * w + x] = sum / float(cnt)
	return out


func _sobel_at(luma: PackedFloat32Array, x: int, y: int, w: int, h: int) -> float:
	if x <= 0 or y <= 0 or x >= w - 1 or y >= h - 1:
		return 0.0
	var tl: float = luma[(y - 1) * w + (x - 1)]
	var t: float = luma[(y - 1) * w + x]
	var tr: float = luma[(y - 1) * w + (x + 1)]
	var l: float = luma[y * w + (x - 1)]
	var r: float = luma[y * w + (x + 1)]
	var bl: float = luma[(y + 1) * w + (x - 1)]
	var b: float = luma[(y + 1) * w + x]
	var br: float = luma[(y + 1) * w + (x + 1)]
	var gx: float = -tl - 2.0 * l - bl + tr + 2.0 * r + br
	var gy: float = -tl - 2.0 * t - tr + bl + 2.0 * b + br
	return clampf(sqrt(gx * gx + gy * gy) / 4.0, 0.0, 1.0)


func _apply_ease(t: float, fn: EaseFunction) -> float:
	t = clampf(t, 0.0, 1.0)
	match fn:
		EaseFunction.LINEAR:
			return t
		EaseFunction.EASE_IN_QUAD:
			return t * t
		EaseFunction.EASE_OUT_QUAD:
			return 1.0 - (1.0 - t) * (1.0 - t)
		EaseFunction.EASE_IN_OUT_QUAD:
			if t < 0.5:
				return 2.0 * t * t
			return 1.0 - pow(-2.0 * t + 2.0, 2.0) * 0.5
		EaseFunction.EASE_IN_CUBIC:
			return t * t * t
		EaseFunction.EASE_OUT_CUBIC:
			return 1.0 - pow(1.0 - t, 3.0)
		EaseFunction.EASE_IN_OUT_CUBIC:
			if t < 0.5:
				return 4.0 * t * t * t
			return 1.0 - pow(-2.0 * t + 2.0, 3.0) * 0.5
		EaseFunction.EASE_IN_QUART:
			return t * t * t * t
		EaseFunction.EASE_OUT_QUART:
			return 1.0 - pow(1.0 - t, 4.0)
		EaseFunction.EASE_IN_OUT_QUART:
			if t < 0.5:
				return 8.0 * t * t * t * t
			return 1.0 - pow(-2.0 * t + 2.0, 4.0) * 0.5
		EaseFunction.EASE_IN_BACK:
			var c1 := 1.70158
			var c3 := c1 + 1.0
			return c3 * t * t * t - c1 * t * t
		EaseFunction.EASE_OUT_BACK:
			var c1 := 1.70158
			var c3 := c1 + 1.0
			return 1.0 + c3 * pow(t - 1.0, 3.0) + c1 * pow(t - 1.0, 2.0)
		EaseFunction.EASE_IN_OUT_BACK:
			var c1 := 1.70158
			var c2 := c1 * 1.525
			if t < 0.5:
				return (pow(2.0 * t, 2.0) * ((c2 + 1.0) * 2.0 * t - c2)) * 0.5
			return (pow(2.0 * t - 2.0, 2.0) * ((c2 + 1.0) * (t * 2.0 - 2.0) + c2) + 2.0) * 0.5
	return t
