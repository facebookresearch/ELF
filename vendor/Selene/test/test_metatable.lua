function call_method()
   instance = get_instance()
   return instance:baz()
end

function access_member()
   instance = get_instance()
   return instance:qux()
end