# RDM - Rikai's Dotfile Manager
A small cli program to initialize dotfiles for people who use multiple machines and environments.

## Basic CLI syntax
`rdm apply|preview <modules> [-f <flags>]`
## Basic Lua syntax
RDM exposes two functions:
- `Read(file)`: Reads a file relative to the script and returns it as a string
- `OptionIsSet(flag)`: Returns true if the flag was passed to RDM, otherwise false

Your inner code can be whatever you want! That's the power of RDM, the only condition is that you should return a table at the end of your module with the output file paths as keys and the file contents as the value.

```lua
local output_content = Read("file_path_relative_to_this_script")

-- Flag code in another file and inserted when the flag is set
if OptionIsSet("work") then
    local work_specific_code = Read("work_specific_code_path")
    output_content = output_content:gsub("#~work", work_specific_code)
end

-- Flag code in the same file, but removed when the flag isn't set
if not OptionIsSet("docker") then
    output_content = output_content:gsub("#~docker.*#~end%-docker", "")
    -- Example of a snippet that would be deleted:
    -- #~docker
    -- alias logs="docker ps --format '{{.Names}}' | fzf | docker logs -f"
    -- #~end-docker
end

return {
    [".my_config_file"] = output_content
}
```

If you are wondering why I use the sytax `#~flag` and `#~end-flag` for replacing, that's because I wanted to! Since every language has their own way of specifying comments and each person has their own preferences on how they would like to split their config code, RDM leaves it up to you to decide how you want to handle your code insertions or deletions :)

> But...what about json files or other formats that don't accept comments?!?!
> 
Since the module reads a file, you manipulate it and then you decide what ends up being the final content, it's really up to you; use any comment or markdown format you want, or none at all if you don't want to, you could use `^*^%$` as a marker for all I care for as long as your output content is valid for your use case.

## Example
`rdm apply hypr kitty zsh -f laptop work es`

This will apply the `hypr`, `kitty` and `zsh` modules and let them know the `laptop`, `work` and `es` flags are active so that the scripts responsible for those modules can make the necessary adjustements.

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
local zshenv = Read(".zshenv")
local zshrc = Read(".zshrc-base")

-- Insert work specific ZSH code
if OptionIsSet("work") then
    local work = Read(".zshrc-work")
    zshrc = zshrc:gsub("#~work", work)
end

return {
    [".zshrc"] = zshrc,
    [".zshenv"] = zshenv
}
```
Then as per the return table, the files `.zshrc` and `.zshenv` will be placed on the home directory of the current user with the contents of the `zshrc` and `zshenv` variables.
