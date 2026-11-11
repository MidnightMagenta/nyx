---@alias Cond "y"|"n"
---@alias Object string[]

---@class (exact) SubDirTable
---@field objs { y: string[], n:  string[] }
---@field add fun(self: SubDirTable, list: string[], cond: Cond?)

local M = {}

local ALLOWED = {
	objs = true,
}
local CONDS = { y = true, n = true }

local methods = {}

---@param self SubDirTable
---@param list string[]
---@param cond Cond?
function methods.add(self, list, cond)
	cond = cond or "y"
	local bucket = CONDS[cond] and self.objs[cond]
	if not bucket then
		error("config value '" .. tostring(cond) .. "' is not allowed", 2)
	end
	table.move(list, 1, #list, #bucket + 1, bucket)
end

local mt = {
	__index = methods,
	__newindex = function(_, k)
		error("unknown key '" .. tostring(k) .. "'")
	end,
}

---@return SubDirTable
function M.new()
	return setmetatable({
		objs = {
			y = {},
			n = {},
		},
	}, mt)
end

---@param t any
---@return boolean
function M.is_subdir_table(t)
	return getmetatable(t) == mt
end

---@param ... SubDirTable
---@return SubDirTable
function M.merge(...)
	local out = M.new()
	for i = 1, select("#", ...) do
		local r = select(i, ...)
		if not M.is_subdir_table(r) then
			error("merge: argument " .. i .. " is not a SubDirTable", 2)
		end
		for cond in pairs(CONDS) do
			local src, dst = r.objs[cond], out.objs[cond]
			table.move(src, 1, #src, #dst + 1, dst)
		end
		-- future fields: merge each with its own rule here
	end
	return out
end

local function listify(x)
	if type(x) == "string" then
		return { x }
	end
	return x or {}
end

---@param subdirs string|string[]
---@param ... any
---@returns SubDirTable
function M.collect(subdirs, ...)
	local collected = M.new()
	for _, p in ipairs(listify(subdirs)) do
		local sddat = mb.subdir(p, ...)
		collected = M.merge(collected, sddat)
	end
	return collected
end

return M
