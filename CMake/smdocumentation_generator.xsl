<?xml version="1.0" encoding="utf8"?>
<!-- XSL used to generate HTMLs from server manager XML
  to run use : xmlpatterns <xsl> <xml> -output <html>
-->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="html"/>
<xsl:template match="ServerManagerConfiguration/ProxyGroup">
  <xsl:variable name="group_name" select="@name" />
  <xsl:for-each select="SourceProxy|Proxy">
    <html>
    <head>
      <meta name="proxy_name"><xsl:value-of select="$group_name" />:<xsl:value-of select="@name" /></meta>
    </head>
    <body>
    <h2><xsl:choose>
      <xsl:when test="@label"> <xsl:value-of select="@label" /> </xsl:when>
      <xsl:otherwise> <xsl:value-of select="@name" /> </xsl:otherwise>
    </xsl:choose> (<xsl:value-of select="@name"/>) </h2>
    <xsl:apply-templates select="Documentation" />
    <table width="97%" border="2px" >
      <tr bgcolor="#9acd32">
        <th>Property</th>
        <th width="60%">Description</th>
        <th width="5%">Default(s)</th>
        <th width="20%">Restrictions</th>
      </tr>
    <xsl:for-each select="DoubleVectorProperty|InputProperty|IntVectorProperty|StringVectorProperty|ProxyProperty|IdTypeVectorProperty">
      <tr>
        <th>
          <xsl:choose>
            <xsl:when test="@label"><xsl:value-of select="@label"/></xsl:when>
            <xsl:otherwise><xsl:value-of select="@name"/></xsl:otherwise>
          </xsl:choose>
        </th>
        <td>
          <xsl:apply-templates select="Documentation" />
        </td>
        <td>
          <div class="defaults">
            <xsl:call-template name="WriteDefaults" />
          </div>
        </td>
        <td>
          <div class="domain">
            <xsl:call-template name="WriteDomain" />
          </div>
        </td>
      </tr>
    </xsl:for-each>
    </table>
    </body>
    </html>
  </xsl:for-each>
</xsl:template>

<!-- Defaults Handler -->
<xsl:template name="WriteDefaults">
  <xsl:if test="@default_values">
    <xsl:value-of select="@default_values" />
  </xsl:if>
</xsl:template>

<!-- Domains Handler -->
<xsl:template name="WriteDomain">
  <xsl:apply-templates select="ArrayListDomain" />
  <xsl:apply-templates select="ArrayRangeDomain" />
  <xsl:apply-templates select="ArraySelectionDomain" />
  <xsl:apply-templates select="BooleanDomain" />
  <xsl:apply-templates select="BoundsDomain" />
  <xsl:apply-templates select="DataTypeDomain" />
  <xsl:apply-templates select="DoubleRangeDomain" />
  <xsl:apply-templates select="IntRangeDomain" />
  <xsl:apply-templates select="EnumerationDomain" />
  <xsl:apply-templates select="ExtentDomain" />
  <xsl:apply-templates select="FieldDataDomain" />
  <xsl:apply-templates select="FileListDomain" />
  <xsl:apply-templates select="FixedTypeDomain" />
  <xsl:apply-templates select="InputArrayDomain" />
  <xsl:apply-templates select="ProxyGroupDomain" />
  <xsl:apply-templates select="ProxyListDomain" />
  <xsl:apply-templates select="StringListDomain" />
</xsl:template>

<xsl:template match="StringListDomain">
  The value(s) can be one of the following:
  <ul>
  <xsl:for-each select="String">
    <li><xsl:value-of select="@value"/></li>
  </xsl:for-each>
  </ul>
</xsl:template>

<xsl:template match="ProxyListDomain">
  The value can be one of the following:
  <ul>
  <xsl:for-each select="Proxy">
    <li><xsl:value-of select="@name"/>
        (<i><xsl:value-of select="@group"/></i>)
    </li>
  </xsl:for-each>
  </ul>
</xsl:template>

<xsl:template match="ProxyGroupDomain">
  <!-- ugh, ignore this domain. It's pretty pointless anyways.-->
</xsl:template>

<xsl:template match="ExtentDomain">
  The value(s) must lie within the structured-extents of the input dataset.
</xsl:template>

<xsl:template match="FieldDataDomain">
  The value must be field array name.
</xsl:template>

<xsl:template match="FileListDomain">
  The value(s) must be a filename (or filenames).
</xsl:template>

<xsl:template match="FixedTypeDomain">
  Once set, the input dataset cannot be changed.
</xsl:template>

<xsl:template match="InputArrayDomain">
  The dataset much contain a field array (
    <xsl:value-of select="@attribute_type"/>)
  <xsl:if test="@number_of_components">
    with <xsl:value-of select="@number_of_components"/> component(s).
  </xsl:if>
</xsl:template>

<xsl:template match="EnumerationDomain">
  The value(s) is an enumeration of the following:
  <ul>
    <xsl:for-each select="Entry">
      <li><xsl:value-of select="@text"/> (
          <xsl:value-of select="@value"/>)</li>
    </xsl:for-each>
  </ul>
</xsl:template>


<xsl:template match="ArrayListDomain[@attribute_type='Scalars']">
  <!-- Handle ArrayListDomain -->
  An array of scalars is required.
</xsl:template>

<xsl:template match="ArrayListDomain[@attribute_type='Vectors']">
  <!-- Handle ArrayListDomain -->
  An array of vectors is required.
</xsl:template>

<xsl:template match="ArrayRangeDomain">
  The value must lie within the range of the selected data array.
</xsl:template>

<xsl:template match="ArraySelectionDomain">
  The list of array names is provided by the reader.
</xsl:template>

<xsl:template match="BoundsDomain[@mode='normal']">
  The value must lie within the bounding box of the dataset.
  <xsl:if test="@default_mode">
    It will default to the <xsl:value-of select="@default_mode" /> in each dimension.
  </xsl:if>
</xsl:template>

<xsl:template match="BoundsDomain[@mode='magnitude']">
  Determine the length of the dataset's diagonal.
  The value must lie within -diagonal length to +diagonal length.
</xsl:template>

<xsl:template match="BoundsDomain[@mode='scaled_extent']">
  The value must be less than the largest dimension of the
  dataset multiplied by a scale factor of
  <xsl:value-of select="@scale_factor" />.
</xsl:template>

<xsl:template match="DoubleRangeDomain">
  <xsl:call-template name="RangeDomain"/>
</xsl:template>

<xsl:template match="IntRangeDomain">
  <xsl:call-template name="RangeDomain"/>
</xsl:template>

<xsl:template name="RangeDomain">
  Value(s) must be in the range (
    <xsl:value-of select="@min"/>,
    <xsl:value-of select="@max"/>).
</xsl:template>

<xsl:template match="DataTypeDomain">
  Accepts input of following types:
  <ul>
  <xsl:for-each select="DataType">
    <li><xsl:value-of select="@value" /> </li>
  </xsl:for-each>
  </ul>
</xsl:template>

<xsl:template match="BooleanDomain">
  Accepts boolean values (0 or 1).
</xsl:template>

<!-- Documentation Handler -->
<xsl:template match="Documentation">
  <xsl:choose>
    <xsl:when test="@long_help">
      <i><p>  <xsl:value-of select="@long_help" /></p></i>
    </xsl:when>
    <xsl:otherwise>
      <xsl:if test="@short_help">
         <i><p>  <xsl:value-of select="@short_help" /></p></i>
      </xsl:if>
    </xsl:otherwise>
  </xsl:choose>
  <div class="description"><xsl:value-of select="." /></div>
</xsl:template>
</xsl:stylesheet>
