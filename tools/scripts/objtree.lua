local _, filepath = ...
mb.track(filepath)

---@class objtree
local M = {}

local function listify(x)
	if type(x) == "string" then
		return { x }
	end
	return x or {}
end

---@class objtree.CompileOpts: mbl.CompileOpts
---@field objdir string Directory under which objects are placed (mirroring the source tree)

---like mbl.compile, but places objects under opts.objdir instead of the build directory,
---so that outputs named after a source directory don't collide with its objects
---@param rule string
---@param srcs string|string[]
---@param opts objtree.CompileOpts
---@return string[]
function M.compile(rule, srcs, opts)
	if not opts or not opts.objdir then
		error("objtree.compile: opts.objdir is required", 2)
	end
	local suffix = opts.suffix or ".o"
	local outs = {}

	for _, p in ipairs(listify(srcs)) do
		local out = mb.build({
			rule = rule,
			out = opts.objdir .. "/" .. p .. suffix,
			implicit_out = opts.implicit_out,
			inputs = p,
			implicit = opts.implicit,
			order_only = opts.order_only,
			vars = opts.vars,
			pool = opts.pool,
		})
		table.move(out, 1, #out, #outs + 1, outs)
	end

	return outs
end

return M
