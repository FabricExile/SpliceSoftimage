' FabricCanvasInspectOpPlugin

function XSILoadPlugin( in_reg )
	in_reg.Author = "Fabric Software Inc."
	in_reg.Name = "FabricCanvasInspectOp"
	in_reg.Email = ""
	in_reg.URL = ""
	in_reg.Major = 1
	in_reg.Minor = 0

	in_reg.RegisterCommand "FabricCanvasInspectOp", "FabricCanvasInspectOp"
	'RegistrationInsertionPoint - do not remove this line

	XSILoadPlugin = true
end function

function XSIUnloadPlugin( in_reg )
	dim strPluginName
	strPluginName = in_reg.Name
	Application.LogMessage strPluginName & " has been unloaded.",siVerbose
	XSIUnloadPlugin = true
end function

function FabricCanvasInspectOp_Init( in_ctxt )
	dim oCmd
	set oCmd = in_ctxt.Source
	oCmd.Description = ""
	oCmd.ReturnValue = true

	FabricCanvasInspectOp_Init = true
end function

function FabricCanvasInspectOp_Execute(  )

	Dim sl, p, plist, o
	set sl = GetValue("SelectionList")
	if sl.Count <> 1 then
		Exit Function
	end if

	set o = GetValue(sl(0) & ".kine.global")
	set plist = o.NestedObjects
	for each p in plist
		if p.type = "CanvasOp" then
			InspectObj p.fullname
			Exit Function
		end if
	next
 
	Dim lft
	Dim op, oOpStack
	for each o in sl
		if o.type = "polymsh" then
			set oOpStack = o.ActivePrimitive.ConstructionHistory
			for each op in oOpStack
				lft = Left(op.name,Len("CanvasOp"))
				if lft = "CanvasOp" then
					InspectObj op.fullname
					Exit Function
				end if
			next
		end if
	next
 
	FabricCanvasInspectOp_Execute = true
end function

