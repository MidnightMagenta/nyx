local _, filepath = ...
mb.track(filepath)

---@class nyxbuild
local nyxbuild = {}

local function read_file(path)
	local f = io.open(path, "r")
	if not f then
		return nil
	end
	local s = f:read("*a")
	f:close()
	return s
end

---@param path string
---@param env string
---@return string
function nyxbuild.get_env_or_cached(path, env)
	local v = read_file(path)
	if v then
		mb.track(path)
		v = v:match("^%s*(.-)%s*$")
		if v ~= "" then
			return v
		end
	end

	v = os.getenv(env)
	if not v or v == "" then
		error("no " .. path .. " found and " .. env .. " environment variable is not set", 2)
	end

	local f, err = io.open(path .. ".tmp", "w")
	if f then
		f:write(v, "\n")
		f:close()
		os.rename(path .. ".tmp", path)
		mb.track(path)
	else
		io.stderr:write("warning: could not write " .. path .. ": ", err, "\n")
	end

	return v
end

return nyxbuild
