<?xml version="1.0"?>

<!-- HTML Stylesheet for UpWatch DocBook XML documents -->

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">

<!-- Do you want to use a CSS file? 0 or 1 -->
<xsl:param name="css.usage" select="1"/>

<!-- If you do, what is the CSS filename? -->
<xsl:param name="css.filename">style.css</xsl:param>

<!-- Should chapters be labeled? 0 or 1 -->
<xsl:param name="chapter.autolabel" select="1"/>

<!-- Should sections be labeled? 0 or 1 -->
<xsl:param name="section.autolabel" select="1"/>

<!-- Should chapter number be in section number? 1.1, 1.2, etc. 0 or 1 -->
<xsl:param name="section.label.includes.component.label" select="1" doc:type="boolean"/>

<!-- If 1, puts first section on separate page from Chapter page -->
<xsl:param name="chunk.first.sections" select="'0'"/>

<!-- To what depth (in sections) should the TOC go? -->
<xsl:param name="toc.section.depth" select="2"/>

<!-- Should graphics be used for admonitions (notes, warnings)? 0 or 1 -->
<xsl:param name="admon.graphics" select="0"/>

<!-- If 1 above, what is path to graphics? Make sure it ends in "/" and
     make sure that it is all on one line. -->
<xsl:param name="admon.graphics.path">images/</xsl:param>

<!-- Again, if 1 above, what is the filename extension for admon graphics? -->
<xsl:param name="admon.graphics.extension" select="'.gif'"/>

<!-- When chunking, use id attribute as filename? 0 or 1 -->
<xsl:param name="use.id.as.filename" select="1"/>

</xsl:stylesheet>

