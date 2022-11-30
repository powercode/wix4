// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.

namespace WixToolset.Netfx
{
    using WixToolset.Data;
    using WixToolset.Netfx.Symbols;

    public static partial class NetfxSymbolDefinitions
    {
        public static readonly IntermediateSymbolDefinition NetFxDotNetSdkCompatibilityCheck = new IntermediateSymbolDefinition(
            NetfxSymbolDefinitionType.NetFxDotNetCompatibilityCheck.ToString(),
            new[]
            {
                new IntermediateFieldDefinition(nameof(NetFxDotNetSdkCompatibilityCheckSymbolFields.Platform), IntermediateFieldType.String),
                new IntermediateFieldDefinition(nameof(NetFxDotNetSdkCompatibilityCheckSymbolFields.Version), IntermediateFieldType.String),
                new IntermediateFieldDefinition(nameof(NetFxDotNetSdkCompatibilityCheckSymbolFields.RollForward), IntermediateFieldType.String),
                new IntermediateFieldDefinition(nameof(NetFxDotNetSdkCompatibilityCheckSymbolFields.Property), IntermediateFieldType.String),
            },
            typeof(NetFxDotNetCompatibilityCheckSymbol));
    }
}

namespace WixToolset.Netfx.Symbols
{
    using WixToolset.Data;

    public enum NetFxDotNetSdkCompatibilityCheckSymbolFields
    {
        Platform,
        Version,
        RollForward,
        Property,
    }

    public class NetFxDotNetSdkCompatibilityCheckSymbol : IntermediateSymbol
    {
        public NetFxDotNetSdkCompatibilityCheckSymbol() : base(NetfxSymbolDefinitions.NetFxDotNetCompatibilityCheck, null, null)
        {
        }

        public NetFxDotNetSdkCompatibilityCheckSymbol(SourceLineNumber sourceLineNumber, Identifier id = null) : base(NetfxSymbolDefinitions.NetFxDotNetCompatibilityCheck, sourceLineNumber, id)
        {
        }

        public IntermediateField this[NetFxDotNetSdkCompatibilityCheckSymbolFields index] => this.Fields[(int)index];

        public string Platform
        {
            get => this.Fields[(int)NetFxDotNetSdkCompatibilityCheckSymbolFields.Platform].AsString();
            set => this.Set((int)NetFxDotNetSdkCompatibilityCheckSymbolFields.Platform, value);
        }

        public string Version
        {
            get => this.Fields[(int)NetFxDotNetSdkCompatibilityCheckSymbolFields.Version].AsString();
            set => this.Set((int)NetFxDotNetSdkCompatibilityCheckSymbolFields.Version, value);
        }

        public string RollForward
        {
            get => this.Fields[(int)NetFxDotNetSdkCompatibilityCheckSymbolFields.RollForward].AsString();
            set => this.Set((int)NetFxDotNetSdkCompatibilityCheckSymbolFields.RollForward, value);
        }

        public string Property
        {
            get => this.Fields[(int)NetFxDotNetSdkCompatibilityCheckSymbolFields.Property].AsString();
            set => this.Set((int)NetFxDotNetSdkCompatibilityCheckSymbolFields.Property, value);
        }
    }
}
