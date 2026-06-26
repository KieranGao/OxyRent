<template>
  <div class="page-container">
    <div class="page-header">
      <h2>Statistics</h2>
      <p>Revenue trends, vehicle status, and brand distribution</p>
    </div>

    <!-- Date Range Filter -->
    <div class="filter-bar">
      <el-date-picker
        v-model="dateRange"
        type="daterange"
        range-separator="to"
        start-placeholder="Start date"
        end-placeholder="End date"
        value-format="YYYY-MM-DD"
        @change="loadRevenueData"
      />
      <el-select v-model="granularity" placeholder="Granularity" style="width: 120px" @change="loadRevenueData">
        <el-option label="Daily" value="daily" />
        <el-option label="Weekly" value="weekly" />
        <el-option label="Monthly" value="monthly" />
      </el-select>
    </div>

    <!-- Charts Grid -->
    <div class="charts-grid">
      <!-- Revenue Trend -->
      <el-card class="chart-card">
        <template #header>
          <span>Revenue Trend</span>
        </template>
        <v-chart :option="revenueOption" :autoresize="true" style="height: 320px" />
      </el-card>

      <!-- Vehicle Status Pie -->
      <el-card class="chart-card">
        <template #header>
          <span>Vehicle Status</span>
        </template>
        <v-chart :option="statusOption" :autoresize="true" style="height: 320px" />
      </el-card>
    </div>

    <div class="charts-grid" style="margin-top: 20px">
      <!-- Brand Distribution -->
      <el-card class="chart-card full-width">
        <template #header>
          <span>Brand Distribution</span>
        </template>
        <v-chart :option="brandOption" :autoresize="true" style="height: 320px" />
      </el-card>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import { use } from 'echarts/core'
import { CanvasRenderer } from 'echarts/renderers'
import { LineChart, PieChart, BarChart } from 'echarts/charts'
import {
  TitleComponent, TooltipComponent, LegendComponent,
  GridComponent,
} from 'echarts/components'
import VChart from 'vue-echarts'
import { getStatsRevenue, getStatsVehicles } from '@/api/finance'
import { ElMessage } from 'element-plus'

use([
  CanvasRenderer,
  LineChart, PieChart, BarChart,
  TitleComponent, TooltipComponent, LegendComponent,
  GridComponent,
])

const dateRange = ref([])
const granularity = ref('daily')

const revenueData = ref([])
const vehicleStats = ref({})

const revenueOption = computed(() => ({
  tooltip: { trigger: 'axis' },
  grid: { left: '3%', right: '4%', bottom: '3%', containLabel: true },
  xAxis: {
    type: 'category',
    data: revenueData.value.map(d => d.date || d.label),
    axisLabel: { rotate: 30 },
  },
  yAxis: { type: 'value', name: 'Revenue (CNY)' },
  series: [{
    name: 'Revenue',
    type: 'line',
    smooth: true,
    areaStyle: { opacity: 0.15 },
    data: revenueData.value.map(d => d.revenue || d.value || 0),
    itemStyle: { color: '#4f6ef7' },
  }],
}))

const statusOption = computed(() => {
  const data = vehicleStats.value.status_distribution || []
  return {
    tooltip: { trigger: 'item' },
    legend: { bottom: '0%' },
    series: [{
      name: 'Vehicle Status',
      type: 'pie',
      radius: ['40%', '70%'],
      avoidLabelOverlap: false,
      itemStyle: { borderRadius: 8, borderColor: '#fff', borderWidth: 2 },
      label: { show: false, position: 'center' },
      emphasis: { label: { show: true, fontSize: 16, fontWeight: 'bold' } },
      labelLine: { show: false },
      data: data.map(d => ({
        name: d.status || d.name,
        value: d.count || d.value,
      })),
    }],
  }
})

const brandOption = computed(() => {
  const data = vehicleStats.value.brand_distribution || []
  return {
    tooltip: { trigger: 'axis' },
    grid: { left: '3%', right: '4%', bottom: '3%', containLabel: true },
    xAxis: {
      type: 'category',
      data: data.map(d => d.brand || d.name),
    },
    yAxis: { type: 'value', name: 'Count' },
    series: [{
      name: 'Count',
      type: 'bar',
      data: data.map(d => d.count || d.value),
      itemStyle: {
        borderRadius: [4, 4, 0, 0],
        color: {
          type: 'linear', x: 0, y: 0, x2: 0, y2: 1,
          colorStops: [
            { offset: 0, color: '#4f6ef7' },
            { offset: 1, color: '#7c5cfc' },
          ],
        },
      },
      barMaxWidth: 48,
    }],
  }
})

async function loadRevenueData() {
  try {
    const params = { granularity: granularity.value }
    if (dateRange.value && dateRange.value.length === 2) {
      params.start_date = dateRange.value[0]
      params.end_date = dateRange.value[1]
    }
    const res = await getStatsRevenue(params)
    if (res.error === 0) {
      revenueData.value = res.data || res.list || []
    }
  } catch {
    ElMessage.error('Failed to load revenue data')
  }
}

async function loadVehicleStats() {
  try {
    const res = await getStatsVehicles()
    if (res.error === 0) {
      vehicleStats.value = res.data || res
    }
  } catch {
    ElMessage.error('Failed to load vehicle stats')
  }
}

onMounted(() => {
  loadRevenueData()
  loadVehicleStats()
})
</script>

<style scoped>
.filter-bar {
  display: flex;
  gap: 12px;
  align-items: center;
  margin-bottom: 24px;
}

.charts-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 20px;
}

.chart-card.full-width {
  grid-column: 1 / -1;
}
</style>
