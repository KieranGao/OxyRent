<template>
  <div class="page-container">
    <!-- 概览统计 -->
    <div class="stat-grid anim-stagger">
      <div class="stat-card">
        <div class="stat-icon gold"><el-icon :size="20"><User /></el-icon></div>
        <div class="stat-value">{{ overview.total_users }}</div>
        <div class="stat-label">注册用户</div>
      </div>
      <div class="stat-card">
        <div class="stat-icon blue"><el-icon :size="20"><Van /></el-icon></div>
        <div class="stat-value">{{ overview.total_vehicles }}</div>
        <div class="stat-label">车辆总数</div>
      </div>
      <div class="stat-card">
        <div class="stat-icon green"><el-icon :size="20"><CircleCheck /></el-icon></div>
        <div class="stat-value">{{ overview.available_vehicles }}</div>
        <div class="stat-label">可用车辆</div>
      </div>
      <div class="stat-card">
        <div class="stat-icon amber"><el-icon :size="20"><Document /></el-icon></div>
        <div class="stat-value">{{ overview.active_orders }}</div>
        <div class="stat-label">进行中订单</div>
      </div>
      <div class="stat-card">
        <div class="stat-icon green"><el-icon :size="20"><CircleCheckFilled /></el-icon></div>
        <div class="stat-value">{{ overview.completed_orders }}</div>
        <div class="stat-label">已完成订单</div>
      </div>
      <div class="stat-card">
        <div class="stat-icon gold"><el-icon :size="20"><Wallet /></el-icon></div>
        <div class="stat-value">{{ formatMoney(overview.total_revenue) }}</div>
        <div class="stat-label">累计收入</div>
      </div>
      <div class="stat-card">
        <div class="stat-icon amber"><el-icon :size="20"><TrendCharts /></el-icon></div>
        <div class="stat-value">{{ formatMoney(overview.month_revenue) }}</div>
        <div class="stat-label">本月收入</div>
      </div>
      <div class="stat-card">
        <div class="stat-icon blue"><el-icon :size="20"><DataLine /></el-icon></div>
        <div class="stat-value">{{ utilizationRate }}%</div>
        <div class="stat-label">车辆利用率</div>
      </div>
    </div>

    <!-- 图表区域 -->
    <div class="charts-grid">
      <!-- 收入趋势 -->
      <div class="glass-card chart-card span-2">
        <div class="glass-card-header">
          <h3>收入趋势</h3>
          <div class="header-controls">
            <el-date-picker
              v-model="dateRange"
              type="daterange"
              range-separator="至"
              start-placeholder="开始日期"
              end-placeholder="结束日期"
              value-format="YYYY-MM-DD"
              style="width: 260px"
              @change="loadRevenueData"
            />
            <el-select v-model="granularity" style="width: 100px" @change="loadRevenueData">
              <el-option label="按日" value="daily" />
              <el-option label="按周" value="weekly" />
              <el-option label="按月" value="monthly" />
            </el-select>
            <span class="text-accent" style="font-size: 14px; font-weight: 500;">总计 {{ formatMoney(revenueTotal) }}</span>
          </div>
        </div>
        <div class="glass-card-body padded">
          <v-chart :option="revenueOption" :autoresize="true" style="height: 320px" />
        </div>
      </div>

      <!-- 车辆状态 -->
      <div class="glass-card chart-card">
        <div class="glass-card-header"><h3>车辆状态分布</h3></div>
        <div class="glass-card-body padded">
          <v-chart :option="statusOption" :autoresize="true" style="height: 300px" />
        </div>
      </div>

      <!-- 订单状态 -->
      <div class="glass-card chart-card">
        <div class="glass-card-header"><h3>订单状态</h3></div>
        <div class="glass-card-body padded">
          <v-chart :option="orderStatusOption" :autoresize="true" style="height: 300px" />
        </div>
      </div>

      <!-- 品牌分布 -->
      <div class="glass-card chart-card span-2">
        <div class="glass-card-header"><h3>品牌分布</h3></div>
        <div class="glass-card-body padded">
          <v-chart :option="brandOption" :autoresize="true" style="height: 300px" />
        </div>
      </div>
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
import { getStatsOverview, getStatsRevenue, getStatsVehicles } from '@/api/finance'
import { ElMessage } from 'element-plus'
import {
  User, Van, Document, Wallet, DataLine, TrendCharts,
  CircleCheck, CircleCheckFilled,
} from '@element-plus/icons-vue'

use([
  CanvasRenderer,
  LineChart, PieChart, BarChart,
  TitleComponent, TooltipComponent, LegendComponent,
  GridComponent,
])

const dateRange = ref([])
const granularity = ref('daily')

const overview = ref({
  total_users: 0,
  total_vehicles: 0,
  available_vehicles: 0,
  active_orders: 0,
  completed_orders: 0,
  total_revenue: 0,
  month_revenue: 0,
})

const revenueData = ref([])
const revenueTotal = ref(0)
const vehicleStats = ref({})

const utilizationRate = computed(() => {
  const total = vehicleStats.value.total || 1
  const rented = vehicleStats.value.rented || 0
  return ((rented / total) * 100).toFixed(1)
})

function formatMoney(val) {
  return (val || 0).toLocaleString('zh-CN', { style: 'currency', currency: 'CNY' })
}

const revenueOption = computed(() => ({
  tooltip: {
    trigger: 'axis',
    backgroundColor: 'rgba(17,17,17,0.9)',
    borderColor: 'rgba(255,255,255,0.06)',
    textStyle: { color: '#f0f0f0' },
  },
  grid: { left: '3%', right: '4%', bottom: '3%', top: '10%', containLabel: true },
  xAxis: {
    type: 'category',
    data: revenueData.value.map(d => d.date),
    axisLine: { lineStyle: { color: '#333' } },
    axisLabel: { color: '#777', rotate: 30 },
  },
  yAxis: {
    type: 'value',
    name: '¥',
    axisLine: { show: false },
    splitLine: { lineStyle: { color: 'rgba(255,255,255,0.04)' } },
    axisLabel: { color: '#777' },
  },
  series: [{
    name: '收入',
    type: 'line',
    smooth: true,
    symbol: 'circle',
    symbolSize: 8,
    areaStyle: {
      color: {
        type: 'linear', x: 0, y: 0, x2: 0, y2: 1,
        colorStops: [
          { offset: 0, color: 'rgba(200, 169, 110, 0.3)' },
          { offset: 1, color: 'rgba(200, 169, 110, 0)' },
        ],
      },
    },
    lineStyle: { color: '#c8a96e', width: 2 },
    itemStyle: { color: '#c8a96e' },
    data: revenueData.value.map(d => d.amount || 0),
  }],
}))

const statusOption = computed(() => {
  const s = vehicleStats.value
  const data = [
    { name: '可用', value: s.available || 0 },
    { name: '在租', value: s.rented || 0 },
    { name: '维保', value: s.maintenance || 0 },
  ]
  return {
    tooltip: {
      trigger: 'item',
      backgroundColor: 'rgba(17,17,17,0.9)',
      borderColor: 'rgba(255,255,255,0.06)',
      textStyle: { color: '#f0f0f0' },
    },
    legend: { bottom: '5%', textStyle: { color: '#777' } },
    series: [{
      name: '车辆状态',
      type: 'pie',
      radius: ['45%', '70%'],
      center: ['50%', '45%'],
      avoidLabelOverlap: false,
      itemStyle: { borderRadius: 10, borderColor: '#111', borderWidth: 3 },
      label: { show: false },
      emphasis: { label: { show: true, fontSize: 18, fontWeight: 'bold', color: '#f0f0f0' } },
      data,
      color: ['#4ade80', '#60a5fa', '#fbbf24'],
    }],
  }
})

const orderStatusOption = computed(() => {
  const data = [
    { name: '进行中', value: overview.value.active_orders || 0 },
    { name: '已完成', value: overview.value.completed_orders || 0 },
  ]
  return {
    tooltip: {
      trigger: 'item',
      backgroundColor: 'rgba(17,17,17,0.9)',
      borderColor: 'rgba(255,255,255,0.06)',
      textStyle: { color: '#f0f0f0' },
    },
    legend: { bottom: '5%', textStyle: { color: '#777' } },
    series: [{
      name: '订单状态',
      type: 'pie',
      radius: ['45%', '70%'],
      center: ['50%', '45%'],
      avoidLabelOverlap: false,
      itemStyle: { borderRadius: 10, borderColor: '#111', borderWidth: 3 },
      label: { show: false },
      emphasis: { label: { show: true, fontSize: 18, fontWeight: 'bold', color: '#f0f0f0' } },
      data,
      color: ['#60a5fa', '#c8a96e'],
    }],
  }
})

const brandOption = computed(() => {
  const data = vehicleStats.value.by_brand || []
  return {
    tooltip: {
      trigger: 'axis',
      backgroundColor: 'rgba(17,17,17,0.9)',
      borderColor: 'rgba(255,255,255,0.06)',
      textStyle: { color: '#f0f0f0' },
    },
    grid: { left: '3%', right: '4%', bottom: '3%', top: '10%', containLabel: true },
    xAxis: {
      type: 'category',
      data: data.map(d => d.brand),
      axisLine: { lineStyle: { color: '#333' } },
      axisLabel: { color: '#777' },
    },
    yAxis: {
      type: 'value',
      name: '数量',
      axisLine: { show: false },
      splitLine: { lineStyle: { color: 'rgba(255,255,255,0.04)' } },
      axisLabel: { color: '#777' },
    },
    series: [{
      name: '数量',
      type: 'bar',
      data: data.map(d => d.count),
      barMaxWidth: 48,
      itemStyle: {
        borderRadius: [6, 6, 0, 0],
        color: {
          type: 'linear', x: 0, y: 0, x2: 0, y2: 1,
          colorStops: [
            { offset: 0, color: '#c8a96e' },
            { offset: 1, color: '#a88b5a' },
          ],
        },
      },
    }],
  }
})

async function loadOverview() {
  try {
    const res = await getStatsOverview()
    if (res.error === 0) {
      overview.value = res
    }
  } catch {}
}

async function loadRevenueData() {
  try {
    const params = { granularity: granularity.value }
    if (dateRange.value && dateRange.value.length === 2) {
      params.start_date = dateRange.value[0]
      params.end_date = dateRange.value[1]
    } else {
      const now = new Date()
      const year = now.getFullYear()
      const month = String(now.getMonth() + 1).padStart(2, '0')
      params.start_date = `${year}-${month}-01`
      params.end_date = `${year}-${month}-31`
    }
    const res = await getStatsRevenue(params)
    if (res.error === 0) {
      revenueData.value = res.items || []
      revenueTotal.value = res.total || 0
    }
  } catch {
    ElMessage.error('加载收入数据失败')
  }
}

async function loadVehicleStats() {
  try {
    const res = await getStatsVehicles()
    if (res.error === 0) {
      vehicleStats.value = res
    }
  } catch {
    ElMessage.error('加载车辆统计失败')
  }
}

onMounted(() => {
  loadOverview()
  loadRevenueData()
  loadVehicleStats()
})
</script>

<style scoped>
.charts-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 16px;
}

.chart-card.span-2 {
  grid-column: span 2;
}

.header-controls {
  display: flex;
  gap: 10px;
  align-items: center;
  flex-wrap: wrap;
}

@media (max-width: 1200px) {
  .charts-grid {
    grid-template-columns: 1fr;
  }
  .chart-card.span-2 {
    grid-column: span 1;
  }
}
</style>
