# RDM - Rikai's Dotfile Manager
A small cli program to manage files and configurations for people who use multiple machines and environments and prefer a modular approach to their setup.

RDM doesn't provide any complex version control, diffing or patching, but instead offers a simple yet powerful way of managing your machine configurations.
It's a tool born from necessity, so I am sure I will not be the only one who will find it useful, if you ever wanted to have a single repository with all your configurations and scripts and be able to easily apply them (and modify them depending on the distribution or system type), I'm sure you will understand where this program comes from!

This is for us, the people that want an alternative because we are outside the use cases of bare git repos, stow and chezmoi.

## Installation
### Arch (AUR)
`yay -Sy rdm`
### Building Manually
1. Clone the repository `git clone https://github.com/Rikaisan/rdm`
2. cd into it `cd rdm`
3. Setup the meson project `meson setup . build --buildtype release`
4. Build the project `meson compile -C build`
5. The binary will be placed in `build/rdm`

## Quickstart
### New Users
1. Initialize RDM with `rdm init`
2. CD into the RDM data dir with `cd $(rdm dir)`
3. Put your files inside of the `home` directory
4. Create RDM modules with the syntax `rdm-module-name.lua`, Lua files ignoring this nomenclature will be ignored.
5. Write your module, the most basic RDM module looks like this:
```lua
function RDM_GetFiles() -- Copy configs for foo and bar
    return {
        [".config/foo"] = Directory("my_configs/foo"),
        [".config/.bar"] = File("my_configs/.bar"),
    }
end
```
6. Preview your module(s) with `rdm preview <module-name>`
### To setup a new machine with rdm
1. Initialize RDM with `rdm clone <repo_url>`
2. Apply modules with `rdm apply [modules...] [-f <flags...>]`

## Basic CLI syntax
`rdm apply [modules...] [-f <flags...>]`
### Example
`rdm apply hyprland wallpapers term -f laptop work es`

This will apply the modules `hyprland`, `wallpapers` and `term` and let them know you passed the flags `laptop`, `work` and `es`, what you do with those is up to you!

- Want a different keymap if the flag `es` was specified since the keyboard layout is different? Go for it!
- Want some files to not be copied over since the `work` flag was specified? You got it.
- Want a different display configuration for laptops? No problem.

## Basic Lua syntax
RDM exposes a few Lua functions that can be found in the `rdmlib.lua` at the RDM data directory, these functions are used to communicate with RDM, letting your module know what you want to do.
These functions are easy to use, but if you ever need examples, feel free to look at [my dotfiles](<https://github.com/Rikaisan/dotfiles>).

Your inner code can be whatever you want! That's the power of RDM, the only condition is that if you use any RDM functions (`RDM_AddModules`, `RDM_Init`, `RDM_GetFiles` and `RDM_Delayed`), they must be global.

```lua
function RDM_AddModules()
    return { "term", "waybar" } -- You can instruct RDM to load other modules, useful if you want to be able to apply them separately or as part of another module for convenience
end

-- Useful for doing work before files are copied over
function RDM_Init()
    if not FlagIsSet("preview") and IsSet("setup") then -- Check to only run this code  when not in preview mode and if the module or flag `setup` was passed
        ForceSpawn("install-dependencies.sh") -- Add exec permission if not present, then execute the file
    else
        print("Skipping dependency installation...")
    end
end

-- The only function that is in charge of handling files
function RDM_GetFiles()
    local outputFiles = {
        [".config/some_dir"] = Directory("configs/some_dir"), -- Use Directory to copy entire directories at once
        [".config/dir2"] = Directory("configs/scripts"):exec("+(*.py|*.sh)"), -- Make some files executable if they match a pattern, extended patterns supported! see fnmatch(3)
        [".local/share/some_app/some_file_with_no_modifications"] = File("files/raw_file"), -- Use File to copy non-text files or files that you don't intend to modify
    }

    local fileContent = Read("file_relative_to_this_script") -- Use Read when you intend to modify the file contents

    -- Have work specific code in another file and insert it when the flag is set
    if FlagIsSet("work") then
        local work_specific_code = Read("work_specific_code_path")
        fileContent = fileContent:gsub("#~work", work_specific_code)
    end

    -- Have flag code in the same file, but removed when the flag isn't set
    if not FlagIsSet("docker") then
        fileContent = fileContent:gsub("#~docker.*#~end%-docker", "")
        -- Example of a snippet that would be deleted:
        -- #~docker
        -- alias logs="docker ps --format '{{.Names}}' | fzf | docker logs -f"
        -- #~end-docker
    end

    outputFiles[".my_config_file"] = fileContent

    -- What if I want this module to provide some files only if another module is also being applied? You got it!
    if ModuleIsSet("foo") then
        outputFiles[".config/foo/plugins/my_plugin"] = File("extras/my_plugin"):exec() -- Both File and Read also support :exec() to mark them as executable, even after modification
    end

    return outputFiles
end

-- Useful for doing work that requires the files to be present
function RDM_Delayed()
    if not FlagIsSet("preview") then -- Check to not run this code in preview mode
        ForceSpawn("setup-services.sh")
    end
end
```

If you are wondering why I use the sytax `#~flag` and `#~end-flag` for replacing, that's because I wanted to! Since every language has their own way of specifying comments and each person has their own preferences on how they would like to split their config code, RDM leaves it up to you to decide how you want to handle your config modifications :)

> But...what about json files or other formats that don't accept comments?!?!
> 
Since the module reads a file, you manipulate it and then you decide what ends up being the final content, it's really up to you; use any comment or markdown format you want, or none at all and use different files if you want to.

Consider the following file structure:
```
zsh/
├─ .zshenv
├─ .zshrc-base
├─ .zshrc-work
├─ rdm-zsh.lua
```
if `rdm-zsh.lua` contains the following code:
```lua
function RDM_GetFiles()
    local zshrc = Read(".zshrc-base")

    -- Insert work specific ZSH code
    if FlagIsSet("work") then
        local work = Read(".zshrc-work")
        zshrc = zshrc:gsub("#~work", work)
    end

    return {
        [".zshrc"] = zshrc,
        [".zshenv"] = File(".zshenv")
    }
end
```
Then as per the return table, the files `.zshrc` and `.zshenv` will be placed on the home directory of the current user with the contents of the modified `zshrc` and the raw `.zshenv` file.

## Philosophy
The goal of RDM is to be flexible and modular; this of course comes at the cost of taking a bit more time to setup and maintain, however, you can make your modules as simple or as complex as you want; you can have a single module that controls everything via flags or you can have a module per application, perhaps you want the best of both worlds and you want to combine them, it's up to you.

I went with that last option after `rdm 1.3.0`, I separated my modules into submodules per-application: `waybar`, `zsh`, `kitty`, etc and then made "group modules" that just work as an alias to add multiple modules at once:
```lua
-- rdm-term.lua
function RDM_AddModules()
    return { "zsh", "kitty", "starship", "bat", "neofetch", "yazi" }
end
```
This way I can just apply `term` for my quick and usual setup or just one of the modules (e.g. `kitty`) if I just need one of them in another machine. This also makes it easier to chain from other modules; I can have my universal `term` setup be added by `hyprland` and therefore if I ever change `term` or my `yazi` configs, it will affect every other modules that include them, be it `hyprland`, `sway` or whatever your setup dictates!