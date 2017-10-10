function cmd_attack()
     -- c_print("In attack!")
     if target_dead() or not see_target() then
         -- c_print("Task finished!")
         done()
         return
     end
     local att_r = unit_att_r()
     if attack_cd_expire() and in_attack_range() then
         -- c_print("Attacking .. ")
         -- Then we need to attack.
         if att_r <= 1.0 then
             send_melee_attack()
         else
             send_range_attack()
         end
     else
         -- c_print("Moving towards target .. ")
         move_toward_target()
    end
end

function cmd_gather()
    if resource_base_gone() then
        done()
        return
    end
    if gather_state == 0 then
        if move_towards_resource() < 1 then
            start_gather_cd()
        end
    elseif gather_state == 1 then
        if gather_cd_expire() then
            harvest_resource()
        end
    elseif gather_state = 2 then
        if move_towards_base() < 1 then
            change_resource()
        end
    end
    update_gather_state();
end

function cmd_build()
    if build_state == 0 then
        if check_close() then
            change_resource()
            start_build_cd()
            update_build_state()
        else
            move_towards_build_p()
        end
    elseif build_state == 1 then
        if build_cd_expire() then
            if check_build_p() then
                actual_build()
                done()
            end
        end
    end
end
