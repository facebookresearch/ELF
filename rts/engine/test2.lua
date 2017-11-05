-- Note that the attribute not in the C structure 
-- will be automatically stored in a LUA table for this unit.

-- Attribute that are not specified but defined in C struct 
-- will use its default value defined in C, or remain undefined.

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


function cmd_attack(env, cmd)
     -- c_print("In attack!")
     if cmd.target:isdead() or not cmd.target:can_be_seen() then
         -- c_print("Task finished!")
         return cmd.COMPLETE
     end
     local att_r = cmd.unit.att_r
     if cmd.unit:cd_expired("attack") and cmd.unit:in_attack_range(cmd.target) then
         -- c_print("Attacking .. ")
         -- Then we need to attack.
         if att_r <= 1.0 then
             cmd:send_cmd("MeleeAttack", cmd.target, cmd.unit.att)
         else
             cmd:send_cmd("EmitBullet", cmd.target, cmd.unit.att, 2)
         end
     else
         -- c_print("Moving towards target .. ")
         cmd:move_towards(cmd.target)
    end
end

function cmd_gather(env, cmd)
    local resource = env:get_resource()
    local base = env:get_base()
    if resource == nil or base == nil then
        return cmd.COMPLETE
    end
    if cmd.gather_state == 0 then
        if cmd:move_towards(resource) < 1 then
            cmd:start_cd("gather")
            cmd.gather_state = 1
        end
    elseif cmd.gather_state == 1 then
        if cmd.unit:cd_expired("gather") then
            cmd:send_cmd("HarvestResource", cmd.resource(), 5) 
            cmd.gather_state = 2 
        end
    elseif cmd.gather_state == 2 then
        if cmd:move_towards(base) < 1 then
            cmd:send_cmd("ChangeResource", cmd.unit:player_id(), 5) 
            cmd.gather_state = 0
        end
    end
end

function cmd_build(env, cmd)
    local cost = env:unit_cost(cmd.build_type)
    if cmd.build_state == 0 then
        if env:dist_sqr(cmd.unit.p, build_p) < kBuildDistStr then
            cmd:send_cmd("ChangeResource", cmd.unit:player_id(), -cost) 
            cmd:start_cd("build")
            cmd.build_state = kBuilding
        else
            if cmd.p:isvalid() then
                local nearby_p = env:get_nearby_location(cmd.p)
                cmd:move_towards_location(nearby_p)
            end
        end
    elseif cmd.build_state == kBuilding then
        if cmd.unit:cd_expired("build") then
            local build_p = cmd.p
            if not build_p:isvalid() then
                build_p = env:get_nearby_location(cmd.unit.p) 
            end
            if build_p:isvalid() then
                cmd:send_cmd("Create", cmd.build_type, build_p, cmd.unit:player_id(), cost) 
                return cmd.COMPLETE
            end
        end
    end
end
