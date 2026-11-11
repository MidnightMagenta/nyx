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

---read a setting from the environment, falling back to its cache file.
---a value taken from the environment refreshes the cache
---@param path string
---@param env string
---@return string
function nyxbuild.get_env_or_cached(path, env)
	local cached = read_file(path)
	cached = cached and cached:match("^%s*(.-)%s*$")

	local v = os.getenv(env)
	if not v or v == "" then
		if not cached or cached == "" then
			error("no " .. path .. " found and " .. env .. " environment variable is not set", 2)
		end
		mb.track(path)
		return cached
	end

	if v ~= cached then
		local f, err = io.open(path .. ".tmp", "w")
		if f then
			f:write(v, "\n")
			f:close()
			os.rename(path .. ".tmp", path)
		else
			io.stderr:write("warning: could not write " .. path .. ": ", err, "\n")
		end
	end
	mb.track(path)

	return v
end

---get a Kconfig string option without the quotes .config stores it with
---@param cfg config
---@param key string
---@param default string?
---@return string
function nyxbuild.cfg_string(cfg, key, default)
	local v = cfg:get(key, default or "")
	return ((v:match('^"(.*)"$') or v):gsub('\\(.)', "%1"))
end

return nyxbuild
