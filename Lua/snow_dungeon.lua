quest snow_dungeon begin
	state start begin
		when 20395.chat."¡Quiero matar al Nemere!" begin
			say_title(mob_name(20395))
			say("Mitos cuentan que el Nemere se encuentra cruzando")
			say("este portal, sin embargo, nadie logro salir para contarnos.")
			say("¿Deseas descubrirlo?")


			local s= select("Si", "No")
			if s == 2 then
				return
			end

			if party.is_party() then
				if not party.is_leader() then
					say("Debes ser líder de un grupo.")
					return
				end

				local pids = {party.get_member_pids()}

				local timerCheck = true
				local levelCheck = true
				local ticketCheck = true

				for i, pid in next, pids, nil do
					-->q.begin_other_pc_block(pid)

					if game.check_event(5, pc.get_empire()) == true then
						if ((get_global_time() - pc.getf("snow_dungeon","last_exit")) < 60*30) then 
							timerCheck = false
						end
					else
						if ((get_global_time() - pc.getf("snow_dungeon","last_exit")) < 60*60) then 
							timerCheck = false
						end
					end

					if pc.count_item(50152) < 1 then
						ticketCheck = false
					end

					if pc.get_level() < 105 then
						levelCheck = false
					end

					-->q.end_other_pc_block()
				end


				if not timerCheck then
					if game.check_event(5, pc.get_empire()) == true then
						say("Deben esperar 30 minutos para reingresar.")
					else
						say("Deben esperar 1 hora para reingresar.")
					end
					return
				end

				if not levelCheck then
					say("Todos los miembros del grupo deben ser nivel 105 o +.")
					return
				end

				if not ticketCheck then
					say("Para acceder, todos los miembros deben")
					say("tener: "..item_name(50152)..".")
            
					return
				end
                
				-->Quitar llaves
				for i, pid in next, pids, nil do
					-->q.begin_other_pc_block(pid)
					pc.remove_item(50152, 1)
					pc.setf("snow_dungeon","last_exit", get_global_time())
					-->q.end_other_pc_block()
				end
			else

				if game.check_event(5, pc.get_empire()) == true then
					if ((get_global_time() - pc.getf("snow_dungeon","last_exit")) < 60*30) then 
						say("Debes esperar 30 minutos para reingresar.")
						return
					end
				else
					if ((get_global_time() - pc.getf("snow_dungeon","last_exit")) < 60*60) then 
						say("Debes esperar 1 hora para reingresar.")
						return
					end
				end

				if pc.count_item(50152) < 1 then
					say("Debes tener al menos un pasaje de entrada en tu inventario.")
					return
				end

				if pc.get_level() < 105 then
					say("Debes ser nivel 105 o + para poder ingresar a esta mazmorra.")
					return
				end

				pc.setf("snow_dungeon","last_exit", get_global_time())
				pc.remove_item(50152, 1)

			end

			onlychat_notice_all("[CH "..get_channel_id().."]: ¡El grupo de "..pc.get_name().." ha ingresado a la Mazmorra de Nemere!")
			GeneralDungeon.NemereAccess()

		end
	end
end

