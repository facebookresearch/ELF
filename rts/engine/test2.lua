-- Note that the attribute not in the C structure 
-- will be automatically stored in a LUA table for this unit.

-- Attribute that are not specified but defined in C struct 
-- will use its default value defined in C, or remain undefined.
--
g_cmd_init_states = {
    ["gather"] = { 
        gather_state = 0
    },
    ["build"] = {
        build_state = 0
    }
}
--
g_cmds = { }
function g_init_cmd_start(cmd_id, name)
    init_state = g_cmd_init_states[name]
    cmd = { _func = g_funcs[name] }
    if init_state ~= nil then
        table.insert(cmd, table.clone(init_state))
    end
    g_cmds[cmd_id] = cmd
end

function g_init_cmd(cmd_id, k, v)
    g_cmds[cmd_id][k] = v
end

function g_run_cmd(cmd_id, env)
    cmd = g_cmds[cmd_id]
    if cmd == nil then 
        return false
    end
    cmd._func(env, cmd)
end

unit_init_attributes = {
    ["BASE"] = { 
         hp = 1000, 
         attack_range = 0, 
         defend = 2
    },
    ["MELEE_ATTACKER"] = { 
         hp = 150, 
         attack_range = 2, 
         custom_type = "melee", 
         attack = 10, 
         defend = 2
    },
    ["RANGE_ATTACKER"] = { 
         hp = 100, 
         attack_range = 5, 
         custom_type = "range", 
         attack = 5, 
         defend = 1
    }   
}

g_funcs = { }
function g_funcs.attack(env, cmd)
     print("In attack!")
     local target = env:unit(cmd.target)
     local u = env:self()

     if target:isdead() or not u:can_see(target) then
         -- c_print("Task finished!")
         return global.COMPLETE
     end
     local att_r = u:att_r()
     if u:cd_expired(global.CD_ATTACK) and env:dist_sqr(target:p()) then
         -- c_print("Attacking .. ")
         -- Then we need to attack.
         if att_r <= 1.0 then
             env:send_cmd_melee_attack(cmd.target, u:att())
         else
             env:send_cmd_emit_bullet(cmd.target, u:att())
         end
     else
         -- c_print("Moving towards target .. ")
         env:move_towards(cmd.target)
    end
end

function g_funcs.gather(env, cmd)
    local resource = env:unit(cmd.resource)
    local base = env:find_closest_base()
    local u = env:self()

    if resource:isdead() or base:isdead() then
        return global.COMPLETE
    end
    if cmd.gather_state == 0 then
        if env:move_towards(resource) < 1 then
            env:start_cd(global.CD_GATHER)
            cmd.gather_state = 1
        end
    elseif cmd.gather_state == 1 then
        if u:cd_expired(global.CD_GATHER) then
            env:send_cmd_harvest_resource(cmd.resource, 5) 
            cmd.gather_state = 2 
        end
    elseif cmd.gather_state == 2 then
        if env:move_towards(base) < 1 then
            env:send_cmd_change_resource(5)
            cmd.gather_state = 0
        end
    end
end

local kBuildDistSqr = 1 

function g_funcs.build(env, cmd)
    local cost = env:unit_cost(cmd.build_type)
    local u = env:self()

    if cmd.build_state == 0 then
        if env:dist_sqr(cmd.build_p) < kBuildDistSqr then
            env:send_cmd_change_resource(u:player_id(), -cost) 
            env:start_cd(global.CD_BUILD)
            cmd.build_state = kBuilding
        else
            if cmd.p:isvalid() then
                local nearby_p = env:get_nearby_location(cmd.p)
                env:move_towards_location(nearby_p)
            end
        end
    elseif cmd.build_state == kBuilding then
        if u:cd_expired(global.CD_BUILD) then
            local build_p = cmd.p
            if not build_p:isvalid() then
                build_p = env:get_nearby_location(u:p()) 
            end
            if build_p:isvalid() then
                env:send_cmd_create(cmd.build_type, build_p, u:player_id(), cost) 
                return global.COMPLETE
            end
        end
    end
end
