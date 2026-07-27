-- Updater-only third-party dependencies.
-- The MoganUpdater target will consume these packages when it is introduced.
if is_plat("windows") then
    add_requires("hdiffpatch v5.1.2", {system=false, configs={shared=false}})
    add_requires("zstd", {system=false})
    add_requires("xxhash", {system=false})
end
